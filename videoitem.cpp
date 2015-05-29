#include "videoitem.h"

#include <QSGTextureProvider>

class VideoRenderer : public QQuickFramebufferObject::Renderer {
    const VideoItem *parent;
    QOpenGLFramebufferObject *fbo;
    QSize renderSize;

public:
    VideoRenderer( const VideoItem *_parent )
        : parent( _parent )
    {

    }


    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
    {


        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // optionally enable multisampling by doing format.setSamples(4);

        //format.setSamples(4);

        fbo = new QOpenGLFramebufferObject( QSize( 1000, 1000), format );


        qDebug() << "IM IN THE RENDERER: handle: " << fbo->handle();


        return fbo;
    }

    void render()
    {
        /*

        glMatrixMode(GL_PROJECTION);

        glMatrixMode( GL_MODELVIEW );
        //glViewport(0, 0, 1920, 1080);
        glLoadIdentity();
        glTranslatef(0.0f,0.0f,-20.0f); //move along z-axis
        glRotatef(30.0,0.0,1.0,0.0); //rotate 30 degress around y-axis
        glRotatef(15.0,1.0,0.0,0.0); //rotate 15 degress around x-axis
        //create viewing cone with near and far clipping planes
                    glMatrixMode(GL_PROJECTION);
                    glLoadIdentity();
                    glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 30.0);

                    glMatrixMode( GL_MODELVIEW );




                //delete color and depth buffer
                   //glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);






                    //create 3D-Cube
                    glBegin(GL_QUADS);

                        //front
                        glColor3f(1.0,0.0,0.0);

                        glVertex3f(1.0,1.0,1.0);
                        glVertex3f(-1.0,1.0,1.0);
                        glVertex3f(-1.0,-1.0,1.0);
                        glVertex3f(1.0,-1.0,1.0);


                        //back

                        glColor3f(0.0,1.0,0.0);

                        glVertex3f(1.0,1.0,-1.0);
                        glVertex3f(-1.0,1.0,-1.0);
                        glVertex3f(-1.0,-1.0,-1.0);
                        glVertex3f(1.0,-1.0,-1.0);


                        //top
                        glColor3f(0.0,0.0,1.0);

                        glVertex3f(-1.0,1.0,1.0);
                        glVertex3f(1.0,1.0,1.0);
                        glVertex3f(1.0,1.0,-1.0);
                        glVertex3f(-1.0,1.0,-1.0);


                        //bottom
                        glColor3f(0.0,1.0,1.0);

                        glVertex3f(1.0,-1.0,1.0);
                        glVertex3f(1.0,-1.0,-1.0);
                        glVertex3f(-1.0,-1.0,-1.0);
                        glVertex3f(-1.0,-1.0,1.0);

                        //right
                        glColor3f(1.0,0.0,1.0);

                        glVertex3f(1.0,1.0,1.0);
                        glVertex3f(1.0,-1.0,1.0);
                        glVertex3f(1.0,-1.0,-1.0);
                        glVertex3f(1.0,1.0,-1.0);


                        //left
                        glColor3f(0.2, 0.3,0.1);

                        glVertex3f(-1.0,1.0,1.0);
                        glVertex3f(-1.0,-1.0,1.0);
                        glVertex3f(-1.0,-1.0,-1.0);
                        glVertex3f(-1.0,1.0,-1.0);


                    glEnd();*/
        if ( parent->run) {
            parent->core()->slotFrame();
        }
                update();
                //parent->window()->resetOpenGLState();
    }

    void synchronize( QQuickFramebufferObject *frameBuf )
    {
        auto *videoItem = static_cast<VideoItem *>( frameBuf );
        videoItem->core()->currentFrameBuffer = fbo;

        renderSize = QSize( videoItem->boundingRect().size().toSize() );

    }

};

QQuickFramebufferObject::Renderer *VideoItem::createRenderer() const
{
    return new VideoRenderer( this );
}

void VideoItem::setCore(Core *core)
{
    qmlCore = core;

    //qmlCore->setParent( nullptr );


    // Run a timer to make core produce a frame at regular intervals, or at vsync
    // coreTimer disabled at the moment due to the granulatiry being 1ms (not good enough)
    // connect( &coreTimer, &QTimer::timeout, &core, &Core::slotFrame );
    connect( this, &VideoItem::signalFrame, core, &Core::slotFrame );

    // Do the next item in the core lifecycle when the state has changed
    connect( core, &Core::signalCoreStateChanged, this, &VideoItem::slotCoreStateChanged );
    connect( this, &VideoItem::signalShutdown, core, &Core::slotShutdown );

    // Get the audio and video timing/format from core once a core and game are loaded,
    // send the data out to each consumer for their own initialization
    connect( core, &Core::signalAVFormat, this, &VideoItem::slotCoreAVFormat );

    connect( core, &Core::signalAudioData, audioOutput, &AudioOutput::slotAudioData );
    connect( core, &Core::signalVideoData, this, &VideoItem::slotVideoData );

    emit coreChanged();
}

VideoItem::VideoItem( QQuickItem *parent ) :
    QQuickFramebufferObject( parent ),
    audioOutput( new AudioOutput() ), audioOutputThread( new QThread( this ) ),
    //core( new Core() ), // coreTimer( new QTimer() ),
    coreThread( nullptr ), coreState( Core::STATEUNINITIALIZED ),
    avInfo(), pixelFormat(),
    corePath( "" ), gamePath( "" ),
    width( 0 ), height( 0 ), pitch( 0 ), coreFPS( 0.0 ), hostFPS( 0.0 ),
    texture( nullptr ),
    frameTimer() {

    setTextureFollowsItemSize(false);

    // Place the objects under VideoItem's control into their own threads
    audioOutput->moveToThread( audioOutputThread );

    // Ensure the objects are cleaned up when it's time to quit and destroyed once their thread is done
    connect( this, &VideoItem::signalShutdown, audioOutput, &AudioOutput::slotShutdown );
    connect( audioOutputThread, &QThread::finished, audioOutput, &AudioOutput::deleteLater );

    // Catch the exit signal and clean up
    connect( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [ = ]() {

        qCDebug( phxController ) << "===========QCoreApplication::aboutToQuit()===========";

        // Shut down Core and the consumers
        emit signalShutdown();

        // Stop processing events in the other threads, then block the main thread until they're finished

        // Stop consumer threads
        audioOutputThread->exit();
        audioOutputThread->wait();
        audioOutputThread->deleteLater();

    } );

    // Connect controller signals and slots

    // Run a timer to make core produce a frame at regular intervals, or at vsync
    // coreTimer disabled at the moment due to the granulatiry being 1ms (not good enough)
    // connect( &coreTimer, &QTimer::timeout, &core, &Core::slotFrame );

    // Do the next item in the core lifecycle when the state has changed


    // Get the audio and video timing/format from core once a core and game are loaded,
    // send the data out to each consumer for their own initialization
    connect( this, &VideoItem::signalAudioFormat, audioOutput, &AudioOutput::slotAudioFormat );
    connect( this, &VideoItem::signalVideoFormat, this, &VideoItem::slotVideoFormat ); // Belongs in both categories

    // Do the next item in the core lifecycle when its state changes
    connect( this, &VideoItem::signalRunChanged, audioOutput, &AudioOutput::slotSetAudioActive );

    // Connect consumer signals and slots

    // Start threads

    audioOutputThread->start();

}

VideoItem::~VideoItem() {

}

//
// Controller methods
//

void VideoItem::slotCoreStateChanged( Core::State newState, Core::Error error ) {

    qCDebug( phxController ) << "slotStateChanged(" << Core::stateToText( newState ) << "," << error << ")";

    coreState = newState;

    switch( newState ) {

        case Core::STATEUNINITIALIZED:
            break;

        // Time to run the game
        case Core::STATEREADY:

            // This is mixing control (coreThread) and consumer (render thread) members...
            coreThread = window()->openglContext()->thread();

            if ( core()->renderType() == Core::RenderType::OpenGL ) {

            core()->currentOpenGLContext = window()->openglContext();
            core()->retroHwCallback()->context_reset();
            }


            // Run a timer to make core produce a frame at regular intervals
            // Disabled at the moment due to the granulatiry being 1ms (not good enough)

//            // Set up and start the frame timer
//            qCDebug( phxController ) << "coreTimer.start("
//                                     << ( double )1 / ( avInfo.timing.fps / 1000 )
//                                     << "ms (core) =" << ( int )( 1 / ( avInfo.timing.fps / 1000 ) )
//                                     << "ms (actual) )";

//            // Stop when the program stops
//            connect( this, &VideoItem::signalShutdown, coreTimer, &QTimer::stop );

//            // Millisecond accuracy on Unix (OS X/Linux)
//            // Multimedia timer accuracy on Windows (better?)
//            coreTimer->setTimerType( Qt::PreciseTimer );

//            // Granulatiry is in the integer range :(
//            coreTimer->start( ( int )( 1 / ( avInfo.timing.fps / 1000 ) ) );

//            // Have the timer run in the same thread as Core
//            // This will mean timeouts are blocking, preventing them from piling up if Core runs too slow
//            coreTimer->moveToThread( coreThread );
//            connect( coreThread, &QThread::finished, coreTimer, &QTimer::deleteLater );

            // Place Core into the render thread
            // Mandatory for OpenGL cores
            // Also prevents massive overhead/performance loss caused by QML effects (like FastBlur)
            core()->setParent( nullptr );
            core()->moveToThread( coreThread );
            connect( coreThread, &QThread::finished, core(), &Core::deleteLater );

            qCDebug( phxController ) << "Begin emulation.";

            // Get core to immediately (sorta) produce the first frame
            emit signalFrame();

            // Force an update to keep the render thread from pausing
            update();

            // Let all the consumers know emulation began
            emit signalRunChanged( true );
            run = true;

            break;

        case Core::STATEFINISHED:
            core()->setParent( nullptr );
            core()->moveToThread( this->thread() );
            break;

        case Core::STATEERROR:
            switch( error ) {
                default:
                    break;
            }

            break;
    }

}

void VideoItem::slotCoreAVFormat( retro_system_av_info avInfo, retro_pixel_format pixelFormat ) {

    this->avInfo = avInfo;
    this->pixelFormat = pixelFormat;

    // TODO: Set this properly, either with testing and averages (RA style) or via EDID (proposed)
    double monitorRefreshRate = QGuiApplication::primaryScreen()->refreshRate();

    emit signalAudioFormat( avInfo.timing.sample_rate, avInfo.timing.fps, monitorRefreshRate );
    emit signalVideoFormat( pixelFormat,
                            avInfo.geometry.max_width,
                            avInfo.geometry.max_height,
                            avInfo.geometry.max_width * ( pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2 ),
                            avInfo.timing.fps, monitorRefreshRate );

}

//
// Consumer methods
//

void VideoItem::slotVideoFormat( retro_pixel_format pixelFormat, int width, int height, int pitch,
                                 double coreFPS, double hostFPS ) {

    qCDebug( phxVideo() ) << "pixelformat =" << pixelFormat << "width =" << width << "height =" << height
                          << "pitch =" << pitch << "coreFPS =" << coreFPS << "hostFPS =" << hostFPS;

    this->pixelFormat = pixelFormat;
    this->width = width;
    this->height = height;
    this->pitch = pitch;
    this->coreFPS = coreFPS;
    this->hostFPS = hostFPS;

}

void VideoItem::slotVideoData( uchar *data, unsigned width, unsigned height, int pitch ) {

    if( texture ) {
        texture->deleteLater();
        texture = nullptr;
    }

    QImage::Format frame_format = retroToQImageFormat( pixelFormat );
    QImage image = QImage( data, width, height, pitch, frame_format );

    texture = window()->createTextureFromImage( image, QQuickWindow::TextureOwnsGLTexture );

    texture->moveToThread( window()->openglContext()->thread() );

    // One half of the vsync render loop
    // Invoke a window redraw now that the texture has changed
    update();

}

void VideoItem::handleWindowChanged( QQuickWindow *window ) {

    if( !window ) {
        return;
    }


    /* #################################
     *
     * DO NOT DELETE THIS COMMENTED CODE!!!
     *
     * #################################
     */

    //parentWindow = window;

    //qDebug() << "handle: " << window->renderTarget()->handle();
    //fbo_t = window->renderTarget()->handle();

    setFlag( QQuickItem::ItemHasContents, true );

    connect( window, &QQuickWindow::openglContextCreated, this, &VideoItem::handleOpenGLContextCreated );

    connect( window, &QQuickWindow::frameSwapped, this, &QQuickItem::update );

}

void VideoItem::handleOpenGLContextCreated( QOpenGLContext *GLContext ) {

    if( !GLContext ) {
        return;
    }

    /* #################################
     *
     * DO NOT DELETE THIS COMMENTED CODE!!!
     *
     * #################################
     */

    //fbo_t = GLContext->defaultFramebufferObject();


    //connect( &fps_timer, &QTimer::timeout, this, &VideoItem::updateFps );

}



QSGNode *VideoItem::updatePaintNode( QSGNode *node, UpdatePaintNodeData *paintData ) {
    Q_UNUSED( paintData );

    auto *textureNode = static_cast<QSGSimpleTextureNode *>( node );
    if ( !textureNode || !core()->getCurrentFramebuffer() )
        return QQuickFramebufferObject::updatePaintNode( node, paintData );
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setRect( boundingRect() );

    auto image = core()->getCurrentFramebuffer()->toImage();
    image = image.scaled((int)boundingRect().width(), (int)boundingRect().height());
    textureNode->setTexture( window()->createTextureFromImage( image ) );
    //qDebug() << "RETURNING TEXTURE: " << core()->getCurrentFramebuffer()->toImage();
    return textureNode;

/*
    if ( core()->renderType() == Core::RenderType::OpenGL ) {
        qDebug() << "Render 3d";
        auto *tex = textureProvider()->texture();
        qDebug() << " Texutre is " << tex;
        return QQuickFramebufferObject::updatePaintNode( node, paintData );
    }

    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>( node );

    if( !textureNode ) {

        textureNode = new QSGSimpleTextureNode;

    }

    // It's not time yet. Show a black rectangle.
    if( coreState != Core::STATEREADY ) {

        generateSimpleTextureNode( Qt::black, textureNode );
        return textureNode;

    }

    // First frame, no video data yet. Tell core to render a frame
    // then display it next time.
    if( !texture ) {

        emit signalFrame();
        generateSimpleTextureNode( Qt::black, textureNode );
        return textureNode;

    }

    static qint64 timeStamp = -1;

    if( timeStamp != -1 ) {

        qreal calculatedFrameRate = ( 1 / ( timeStamp / 1000000.0 ) ) * 1000.0;
        int difference = calculatedFrameRate > coreFPS ?
                         calculatedFrameRate - coreFPS :
                         coreFPS - calculatedFrameRate;
        Q_UNUSED( difference );

    }

    timeStamp = frameTimer.nsecsElapsed();
    frameTimer.start();

    textureNode->setTexture( texture );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Linear );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically |
            QSGSimpleTextureNode::MirrorHorizontally );

    // One half of the vsync loop
    // Now that the texture is sent out to be drawn, tell core to make a new frame
    if( coreState == Core::STATEREADY ) {
        emit signalFrame();
    }


    return textureNode;
    */

}


void VideoItem::componentComplete() {

    QQuickItem::componentComplete();

    setFlag( QQuickItem::ItemHasContents, true );

    update();

}

void VideoItem::generateSimpleTextureNode( Qt::GlobalColor globalColor, QSGSimpleTextureNode *textureNode ) {

    QImage image( boundingRect().size().toSize(), QImage::Format_ARGB32 );
    image.fill( globalColor );

    QSGTexture *texture = window()->createTextureFromImage( image );
    textureNode->setTexture( texture );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Nearest );

}

QImage::Format VideoItem::retroToQImageFormat( retro_pixel_format fmt ) {

    static QImage::Format format_table[3] = {
        QImage::Format_RGB16,   // RETRO_PIXEL_FORMAT_0RGB1555
        QImage::Format_RGB32,   // RETRO_PIXEL_FORMAT_XRGB8888
        QImage::Format_RGB16    // RETRO_PIXEL_FORMAT_RGB565
    };

    if( fmt >= 0 && fmt < ( sizeof( format_table ) / sizeof( QImage::Format ) ) ) {
        return format_table[fmt];
    }

    return QImage::Format_Invalid;

}

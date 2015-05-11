#include "videoitem.h"

VideoItem::VideoItem() :
    audioOutput(), audioOutputThread(),
    core(), coreTimer(), coreThread(), coreState( Core::STATEUNINITIALIZED ),
    avInfo(), pixelFormat(),
    corePath( "" ), gamePath( "" ),
    width( 0 ), height( 0 ), pitch( 0 ), coreFPS( 0.0 ), hostFPS( 0.0 ),
    texture( nullptr ),
    frameTimer() {

    // Place the objects under VideoItem's control into their own threads

    core.moveToThread( &coreThread );
    audioOutput.moveToThread( &audioOutputThread );

    // Connect controller signals and slots

    // Run a timer to make core produce a frame at regular intervals
    // Disabled at the moment due to the granulatiry being 1ms (not good enough)
    // connect( &coreTimer, &QTimer::timeout, &core, &Core::slotFrame );

    connect( &core, &Core::signalCoreStateChanged, this, &VideoItem::slotCoreStateChanged );
    connect( this, &VideoItem::signalLoadCore, &core, &Core::slotLoadCore );
    connect( this, &VideoItem::signalLoadGame, &core, &Core::slotLoadGame );
    connect( &core, &Core::signalAVFormat, this, &VideoItem::slotCoreAVFormat );
    connect( this, &VideoItem::signalAudioFormat, &audioOutput, &AudioOutput::slotAudioFormat );
    connect( this, &VideoItem::signalRunChanged, &audioOutput, &AudioOutput::slotSetAudioActive );
    connect( this, &VideoItem::signalVideoFormat, this, &VideoItem::slotVideoFormat ); // Belongs in both categories
    connect( this, &VideoItem::signalFrame, &core, &Core::slotFrame );

    // Connect consumer signals and slots

    connect( &core, &Core::signalAudioData, &audioOutput, &AudioOutput::slotAudioData );
    connect( &core, &Core::signalVideoData, this, &VideoItem::slotVideoData );

    // Start threads

    coreThread.start();
    audioOutputThread.start();

}

VideoItem::~VideoItem() {

    // Tell objects living in other threads it's time to shut down and be ready to get destroyed
    // Some of these connections are blocking to ensure they'll get properly stopped
    emit signalDestroy();

    // Stop processing events in the other threads, then block the main thread until they're finished
    coreThread.exit();
    coreThread.wait();
    audioOutputThread.exit();
    audioOutputThread.wait();

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

            // Run a timer to make core produce a frame at regular intervals
            // Disabled at the moment due to the granulatiry being 1ms (not good enough)

            /*
            // Set up and start the frame timer
            qCDebug( phxController ) << "\tcoreTimer.start(" << ( double )1 / ( avInfo.timing.fps / 1000 ) << "ms (core) =" << ( int )( 1 / ( avInfo.timing.fps / 1000 ) ) << "ms (actual) )";

            // Stop when the program stops
            connect( this, &VideoItem::signalDestroy, &coreTimer, &QTimer::stop, Qt::BlockingQueuedConnection );

            // Millisecond accuracy on Unix (OS X/Linux)
            // Multimedia timer accuracy on Windows (better?)
            coreTimer.setTimerType( Qt::PreciseTimer );

            // Granulatiry is in the integer range :(
            coreTimer.start( ( int )( 1 / ( avInfo.timing.fps / 1000 ) ) );

            // Have the timer run in the same thread as Core
            // This will mean timeouts are blocking, preventing them from piling up if Core runs too slow
            coreTimer.moveToThread( &coreThread );
            */

            qCDebug( phxController ) << "Begin emulation.";

            // Get core to immediately (sorta) produce the first frame
            emit signalFrame();

            // Let all the consumers know emulation began
            emit signalRunChanged( true );

            break;

        case Core::STATEFINISHED:
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
    double monitorRefreshRate = 60.0;

    emit signalAudioFormat( avInfo.timing.sample_rate, avInfo.timing.fps, monitorRefreshRate );
    emit signalVideoFormat( pixelFormat,
                            avInfo.geometry.max_width,
                            avInfo.geometry.max_height,
                            avInfo.geometry.max_width
                            * ( pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2 ),
                            avInfo.timing.fps, monitorRefreshRate );

}

void VideoItem::setCore( QString libretroCore ) {

    corePath = QUrl( libretroCore ).toLocalFile();

    // qCDebug( phxController ) << "emit signalLoadCore(" << corePath << ")";
    emit signalLoadCore( corePath );

}

void VideoItem::setGame( QString game ) {

    gamePath = QUrl( game ).toLocalFile();

    // qCDebug( phxController ) << "emit signalLoadGame(" << gamePath << ")";
    emit signalLoadGame( gamePath );

}

//
// Consumer methods
//

void VideoItem::slotVideoFormat( retro_pixel_format pixelFormat, int width, int height, int pitch, double coreFPS, double hostFPS ) {

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
    texture = window()->createTextureFromImage( QImage( data,
              width,
              height,
              pitch,
              frame_format )
              , QQuickWindow::TextureOwnsGLTexture );

    texture->moveToThread( window()->openglContext()->thread() );

    // One half of the vsync render loop
    // Invoke a window redraw now that the texture is changed
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
        int difference = calculatedFrameRate > coreFPS ? calculatedFrameRate - coreFPS : coreFPS - calculatedFrameRate;
        Q_UNUSED( difference );

    }

    timeStamp = frameTimer.nsecsElapsed();
    frameTimer.start();

    textureNode->setTexture( texture );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Linear );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically | QSGSimpleTextureNode::MirrorHorizontally );

    // One half of the vsync loop
    // Now that the texture is sent out to be drawn, tell core to make a new frame
    if( coreState == Core::STATEREADY ) {
        emit signalFrame();
    }

    return textureNode;

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

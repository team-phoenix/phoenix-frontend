#include "videoitem.h"

VideoItem::VideoItem() :
    texture( nullptr ) {

    // Place the objects under VideoItem's control into their own threads
    core.moveToThread( &coreThread );
    // audioOutput.moveToThread( &audioOutputThread );

    // Connect controller signals and slots
    connect( this, &VideoItem::signalLoadCore, &core, &Core::slotLoadCore );
    connect( this, &VideoItem::signalLoadGame, &core, &Core::slotLoadGame );
    // connect( this, &VideoItem::signalAudioFormat, &audioOutput, &AudioOutput::slotAudioFormat );
    connect( this, &VideoItem::signalVideoFormat, this, &VideoItem::slotVideoFormat );
    connect( this, &VideoItem::signalFrame, &core, &Core::slotFrame );
}

VideoItem::~VideoItem() {

    coreThread.requestInterruption();
    coreThread.wait();

}

//
// Controller methods
//

void VideoItem::slotCoreStateChanged( Core::State newState, Core::Error error ) {
    coreState = newState;

    switch( newState ) {
        case Core::STATEUNINITIALIZED:
            break;

        case Core::STATEREADY:
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

    emit signalAudioFormat( avInfo.timing.sample_rate, avInfo.timing.fps, 60.0 );
    emit signalVideoFormat( pixelFormat,
                            avInfo.geometry.max_width,
                            avInfo.geometry.max_height,
                            avInfo.geometry.max_width
                            * ( pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2 ),
                            avInfo.timing.fps, 60.0 );

}

void VideoItem::setCore( QString libretroCore ) {

    libretroCore = QUrl( libretroCore ).toLocalFile();
    corePath = libretroCore;
    emit signalLoadCore( corePath.toUtf8().constData() );

}

void VideoItem::setGame( QString game ) {

    game = QUrl( game ).toLocalFile();
    gamePath = game;
    emit signalLoadGame( gamePath.toUtf8().constData() );

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

void VideoItem::slotVideoData( uchar *data, unsigned width, unsigned height, size_t pitch ) {

    if( texture ) {
        texture->deleteLater();
    }

    QImage::Format frame_format = retroToQImageFormat( pixelFormat );
    texture = window()->createTextureFromImage( QImage( data,
              width,
              height,
              pitch,
              frame_format )
              , QQuickWindow::TextureOwnsGLTexture );

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
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setOwnsTexture( true );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically | QSGSimpleTextureNode::MirrorHorizontally );

    emit signalFrame();

    return textureNode;

}

void VideoItem::componentComplete() {

    QQuickItem::componentComplete();

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
    textureNode->setOwnsTexture( true );

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

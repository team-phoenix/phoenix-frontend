#include "videoitem.h"

VideoItem::VideoItem( QQuickItem *parent ) :
    QQuickItem( parent ),
    qmlInputManager( nullptr ),
    audioOutput( new AudioOutput() ), audioOutputThread( new QThread( this ) ),
    core( new Core() ), // coreTimer( new QTimer() ),
    coreThread( new QThread( this ) ), coreState( Core::STATEUNINITIALIZED ),
    avInfo(), pixelFormat(),
    corePath( "" ), gamePath( "" ),
    width( 0 ), height( 0 ), pitch( 0 ), coreFPS( 0.0 ), hostFPS( 0.0 ),
    texture( nullptr ),
    frameTimer(), looper( new Looper() ), looperThread( new QThread( this ) ) {

    setFlag( QQuickItem::ItemHasContents, true );

    // Place the objects under VideoItem's control into their own threads
    core->moveToThread( coreThread );
    looper->moveToThread( coreThread );
    audioOutput->moveToThread( audioOutputThread );

    // Ensure the objects are cleaned up when it's time to quit and destroyed once their thread is done
    connect( this, &VideoItem::signalShutdown, audioOutput, &AudioOutput::slotShutdown );
    connect( this, &VideoItem::signalShutdown, looper, &Looper::endLoop );
    connect( this, &VideoItem::signalShutdown, core, &Core::slotShutdown );
    connect( coreThread, &QThread::finished, core, &Core::deleteLater );
    connect( audioOutputThread, &QThread::finished, audioOutput, &AudioOutput::deleteLater );

    // Catch the user exit signal and clean up
    connect( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [ = ]() {

        qCDebug( phxController ) << "===========QCoreApplication::aboutToQuit()===========";

        // Shut down Core and the consumers
        emit signalShutdown();

        qCDebug( phxController ) << "Shutting down and deleting coreThread...";

        // Stop processing events in the other threads, then block the main thread until they're finished
        coreThread->exit();
        coreThread->wait();
        coreThread->deleteLater();

        qCDebug( phxController ) << "Shutting down and deleting consumer threads...";

        // Stop consumer threads
        audioOutputThread->exit();
        audioOutputThread->wait();
        audioOutputThread->deleteLater();

        qCDebug( phxController ) << "Finished. Ready to quit.";

    } );

    // Connect controller signals and slots

    // Run a timer to make core produce a frame at the core-provided framerate (coreFPS), or at vsync (hostFPS)
    connect( looper, &Looper::signalFrame, core, &Core::slotFrame );
    connect( this, &VideoItem::signalBeginLooper, looper, &Looper::beginLoop );

    // Do the next item in the core lifecycle when the state has changed
    connect( core, &Core::signalCoreStateChanged, this, &VideoItem::slotCoreStateChanged );

    // Load a core and a game
    connect( this, &VideoItem::signalLoadCore, core, &Core::slotLoadCore );
    connect( this, &VideoItem::signalLoadGame, core, &Core::slotLoadGame );

    // Get the audio and video timing/format from core once a core and game are loaded,
    // send the data out to each consumer for their own initialization
    connect( core, &Core::signalAVFormat, this, &VideoItem::slotCoreAVFormat );
    connect( this, &VideoItem::signalAudioFormat, audioOutput, &AudioOutput::slotAudioFormat );
    connect( this, &VideoItem::signalVideoFormat, this, &VideoItem::slotVideoFormat ); // Belongs in both categories

    // Do the next item in the core lifecycle when its state changes
    connect( this, &VideoItem::signalRunChanged, audioOutput, &AudioOutput::slotSetAudioActive );

    // Connect consumer signals and slots

    connect( core, &Core::signalAudioData, audioOutput, &AudioOutput::slotAudioData );
    connect( core, &Core::signalVideoData, this, &VideoItem::slotVideoData );

    // Start threads

    audioOutputThread->start();
    // looperThread->start( );
    coreThread->start( QThread::TimeCriticalPriority );

}

VideoItem::~VideoItem() {

}

//
// Controller methods
//

InputManager *VideoItem::inputManager() const {
    return qmlInputManager;
}

void VideoItem::setInputManager( InputManager *manager ) {

    if( manager != qmlInputManager ) {

        qmlInputManager = manager;
        core->inputManager = qmlInputManager;

        connect( this, &VideoItem::signalRunChanged, qmlInputManager, &InputManager::setRun, Qt::DirectConnection );

        emit inputManagerChanged();

    }

}

void VideoItem::slotCoreStateChanged( Core::State newState, Core::Error error ) {

    qCDebug( phxController ) << "slotStateChanged(" << Core::stateToText( newState ) << "," << error << ")";

    coreState = newState;

    switch( newState ) {

        case Core::STATEUNINITIALIZED:
            break;

        // Time to run the game
        case Core::STATEREADY:

            qCDebug( phxController ) << "Begin emulation.";

            // Let all the consumers know emulation began
            emit signalRunChanged( true );

            // Get core to immediately (sorta) produce the first frame
            emit signalBeginLooper( ( 1.0 / hostFPS ) * 1000.0 );

            // Force an update to keep the render thread from pausing
            update();

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
    double monitorRefreshRate = avInfo.timing.fps;

    emit signalAudioFormat( avInfo.timing.sample_rate, avInfo.timing.fps, monitorRefreshRate );
    emit signalVideoFormat( pixelFormat,
                            avInfo.geometry.max_width,
                            avInfo.geometry.max_height,
                            avInfo.geometry.max_width * ( pixelFormat == RETRO_PIXEL_FORMAT_XRGB8888 ? 4 : 2 ),
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

    // Update the frame
    update();

}

void VideoItem::handleWindowChanged( QQuickWindow *window ) {

    if( !window ) {
        return;
    }


}

void VideoItem::keyPressEvent( QKeyEvent *event ) {
    qmlInputManager->keyboard->insert( event->key(), true );
}

void VideoItem::keyReleaseEvent( QKeyEvent *event ) {
    qmlInputManager->keyboard->insert( event->key() , false );
}

bool VideoItem::limitFrameRate() {
    return false;
}

QSGNode *VideoItem::updatePaintNode( QSGNode *node, UpdatePaintNodeData *paintData ) {
    Q_UNUSED( paintData );

    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>( node );

    if( !textureNode ) {
        textureNode = new QSGSimpleTextureNode;
    }

    // It's not time yet. Show a black rectangle.
    if( coreState != Core::STATEREADY ) {
        return QQuickItem::updatePaintNode( textureNode, paintData );
    }

    // First frame, no video data yet. Tell core to render a frame
    // then display it next time.
    if( !texture ) {
        emit signalFrame();
        return QQuickItem::updatePaintNode( textureNode, paintData );
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
        // emit signalFrame();
    }

    return textureNode;

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

#include "videoitem.h"

#include <QSGSimpleRectNode>
#include <QFileInfo>
#include <QFile>
#include <QQuickWindow>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>


VideoItem::VideoItem() :
    coreTimer( nullptr ),
    renderReady( false ),
    gameReady( false ),
    libretroCoreReady( false ),
    texture( nullptr ) {
    //coreTimer.setParent( &core );
    coreTimer.moveToThread( &coreThread );
    coreTimer.setTimerType( Qt::PreciseTimer );

    core.moveToThread( &coreThread );
    audio.moveToThread( &coreThread );
    audioBuffer.moveToThread( &coreThread );

    audio.audioTimer = &coreTimer;

    connect( &coreThread, &QThread::started, &audio, &Audio::slotThreadStarted );
    //connect( &coreThread, &QThread::started, this, &VideoItem::startCoreTimer, Qt::DirectConnection );
    //connect( &coreThread, &QThread::started, core, &Core::startTimer );
    connect( &coreThread, &QThread::started, &audio, [ this ]  {
        audio.slotRunChanged( true );
    } );

    //connect( &coreTimer, &QTimer::timeout, core, &Core::slotDoFrame );

    connect( &core, &Core::signalRenderFrame, this, &VideoItem::update );
    connect( this, &VideoItem::signalDoFrame, &core, &Core::slotDoFrame );


    connect( &audioBuffer, &AudioBuffer::signalReadReady, &audio, &Audio::slotHandlePeriodTimer );
    //connect( core, &Core::signalRenderFrame, audio, &Audio::slotHandlePeriodTimer );
    connect( this, &VideoItem::windowChanged, this, &VideoItem::handleWindowChanged );

    connect( &core, &Core::signalVideoDataReady, this, &VideoItem::createTexture );
}

VideoItem::~VideoItem() {
    coreThread.exit();
    coreThread.wait();

    audioThread.exit();
    audioThread.wait();

}

void VideoItem::createTexture( uchar *data, unsigned width, unsigned height, int pitch ) {

    QImage::Format frame_format = retroToQImageFormat( core.getPixelFormat() );
    texture = window()->createTextureFromImage( QImage( std::move( data ),
              width,
              height,
              pitch,
              frame_format )
              , QQuickWindow::TextureOwnsGLTexture );

}

void VideoItem::refresh() {
    if( gameReady && libretroCoreReady ) {

        core.setAudioBuffer( &audioBuffer );

        audio.setDefaultFormat( core.getSampleRate() );
        //core->slotStartCoreThread( QThread::TimeCriticalPriority );

        //core->slotHandleCoreStateChanged( Core::Running );
        renderReady = true;

        startThread( VideoItem::CoreThread, QThread::TimeCriticalPriority );
        audio.slotRunChanged( true );

    }
}

void VideoItem::componentComplete() {
    QQuickItem::componentComplete();

    renderReady = true;

    if( renderReady ) {
        update();
    }

}


void VideoItem::handleWindowChanged( QQuickWindow *window ) {
    if( !window ) {
        return;
    }

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

    //fbo_t = GLContext->defaultFramebufferObject();

    //connect( &fps_timer, &QTimer::timeout, this, &VideoItem::updateFps );

}

QString VideoItem::game() const {
    return qmlGame;
}

QString VideoItem::libretroCore() const {
    return qmlLibretroCore;
}

void VideoItem::setLibretroCore( QString libretroCore ) {
    libretroCore = QUrl( libretroCore ).toLocalFile();
    qmlLibretroCore = libretroCore;

    /*if ( core->state() == Core::Running ) {
        frameDataQueue.clear();
        renderReady = false;
        gameReady = false;
        core->slotHandleCoreStateChanged( Core::Unloaded );
    }*/

    libretroCoreReady = core.loadCore( libretroCore.toUtf8().constData() );

    emit libretroCoreChanged();

    refresh();
}

void VideoItem::setGame( QString game ) {
    game = QUrl( game ).toLocalFile();
    qmlGame = game;

    //if ( core->state() == Core::Running ) {
    //  setLibretroCore( qmlLibretroCore );
    //}

    gameReady = core.loadGame( game.toUtf8().constData() );

    emit gameChanged();

    refresh();

}

/*void VideoItem::slotHandleFrameData( FrameData *videoFrame )
{
    // The core thread takes a little time to end, in this time the
    // callbacks are still being fired, we need to verify the core is intact or
    // else we will get strange errors.

    if ( core->state() != Core::Running ) {
        delete videoFrame;
        return;
    }

    frameDataQueue.enqueue( videoFrame );
    //qDebug() << "Video Frame: size( " << size << " )";
    update();
}*/

void VideoItem::simpleTextureNode( Qt::GlobalColor globalColor, QSGSimpleTextureNode *textureNode ) {
    QImage image( boundingRect().size().toSize(), QImage::Format_ARGB32 );
    image.fill( globalColor );

    QSGTexture *texture = window()->createTextureFromImage( image );
    textureNode->setTexture( texture );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setOwnsTexture( true );
}
QSGNode *VideoItem::updatePaintNode( QSGNode *node, UpdatePaintNodeData *paintData ) {
    Q_UNUSED( paintData );

    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>( node );

    if( !textureNode ) {
        textureNode = new QSGSimpleTextureNode;
    }

    if( !renderReady ) {

        // This will be false if the scene graph loads before the core.
        // Which can happen more than you think.

        // If that's the case then just give us a black rectangle.
        simpleTextureNode( Qt::black, textureNode );
        return textureNode;

    }

    if( !texture ) {
        emit signalDoFrame();
        simpleTextureNode( Qt::black, textureNode );
        return textureNode;

    }

    static qint64 timeStamp = -1;

    if( timeStamp != -1 ) {
        qreal calculatedFrameRate = ( 1 / ( timeStamp / 1000000.0 ) ) * 1000.0;
        int difference = calculatedFrameRate > core.getFps() ? calculatedFrameRate - core.getFps() : core.getFps() - calculatedFrameRate;
        Q_UNUSED( difference );
        //qDebug() << "FrameRate: " <<  difference << " coreFps: " << core->getFps() << " calculatedFPS: " << calculatedFrameRate;

        //frameTimer.hasExpired()
        //qDebug() << (qreal)( 1 / core->getFps() ) * 1000;

        //if ( difference > 1 / 60 ) {

        //}

    }

    timeStamp = frameTimer.nsecsElapsed();
    frameTimer.start();

    if( core.isDupeFrame() ) {
        return textureNode;
    }

    textureNode->setTexture( texture );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setOwnsTexture( true );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically | QSGSimpleTextureNode::MirrorHorizontally );

    emit signalDoFrame();

    return textureNode;
}

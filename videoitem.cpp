#include "videoitem.h"

#include <QSGSimpleRectNode>
#include <QFileInfo>
#include <QFile>
#include <QQuickWindow>
#include <QDebug>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

VideoItem::VideoItem() :
    core( new Core ),
    audio( new Audio ),
    renderReady( false ),
    gameReady( false ),
    libretroCoreReady( false )
{
    Q_CHECK_PTR( audio );
    Q_CHECK_PTR ( core );

    connect( this, &VideoItem::windowChanged, this, &VideoItem::handleWindowChanged );
    connect( core, &Core::signalVideoRefreshCallback, this, &VideoItem::slotHandleFrameData );
    connect( core, &Core::signalCallbackAudioData, this, &VideoItem::slotHandleAudioData );
}

VideoItem::~VideoItem()
{

}

void VideoItem::refresh()
{
    if ( gameReady && libretroCoreReady ) {

        renderReady = true;
        core->slotStartCoreThread( QThread::TimeCriticalPriority );
        updateAudioFormat();
        audio->startAudioThread();

    }
}

void VideoItem::componentComplete()
{
    QQuickItem::componentComplete();

    //setLibretroCore( "/Users/lee/Desktop/vbam_libretro.dylib" );
    //setGame( "/Users/lee/Desktop/GBA/Golden Sun.gba" );

    renderReady = true;

    if (renderReady) {
        update();
    }

}


void VideoItem::handleWindowChanged(QQuickWindow *window)
{
    if (!window)
        return;


    /* #################################
     *
     * DO NOT DELETE THIS COMMENTED CODE!!!
     *
     * #################################
     */

    //parentWindow = window;

    //qDebug() << "handle: " << window->renderTarget()->handle();
    //fbo_t = window->renderTarget()->handle();

    setFlag( QQuickItem::ItemHasContents, true);

    connect( window, &QQuickWindow::openglContextCreated, this, &VideoItem::handleOpenGLContextCreated );


    //connect(window, &QQuickWindow::frameSwapped, this, &QQuickItem::update);

}

void VideoItem::handleOpenGLContextCreated( QOpenGLContext *GLContext)
{
    if (!GLContext)
        return;

    /* #################################
     *
     * DO NOT DELETE THIS COMMENTED CODE!!!
     *
     * #################################
     */

    //fbo_t = GLContext->defaultFramebufferObject();


    //connect( &fps_timer, &QTimer::timeout, this, &VideoItem::updateFps );

}

QString VideoItem::game() const
{
    return qmlGame;
}

QString VideoItem::libretroCore() const
{
    return qmlLibretroCore;
}

void VideoItem::setLibretroCore(QString libretroCore)
{
    qDebug() << libretroCore;
    libretroCore = libretroCore.remove("file://");
    qmlLibretroCore = libretroCore;
    libretroCoreReady = core->loadCore(libretroCore.toUtf8().constData());

    emit libretroCoreChanged();

    refresh();
}

void VideoItem::setGame(QString game)
{
    game = game.remove( "file://" );
    qmlGame = game;
    gameReady = core->loadGame( game.toUtf8().constData() );

    emit gameChanged();

    refresh();
}

void VideoItem::updateAudioFormat()
{
    QAudioFormat format;
    format.setSampleSize( 16 );
    format.setSampleRate( core->getSampleRate() );
    format.setChannelCount( 2 );
    format.setSampleType( QAudioFormat::SignedInt );
    format.setByteOrder( QAudioFormat::LittleEndian );
    format.setCodec( "audio/pcm" );
    audio->setInFormat( format );
}

void VideoItem::slotHandleAudioData( AudioData *audioFrame )
{
    audio->getAudioBuf()->write( audioFrame->data, audioFrame->size );
    delete audioFrame;
}

void VideoItem::slotHandleFrameData( FrameData *videoFrame )
{
    frameDataQueue.enqueue(videoFrame);
    update();
}

void VideoItem::simpleTextureNode( Qt::GlobalColor globalColor, QSGSimpleTextureNode *textureNode )
{
    QImage image( boundingRect().size().toSize(), QImage::Format_ARGB32 );
    image.fill( globalColor );

    QSGTexture *texture = window()->createTextureFromImage( image );
    textureNode->setTexture( texture );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setOwnsTexture( true );
}
QSGNode* VideoItem::updatePaintNode( QSGNode *node, UpdatePaintNodeData *paintData )
{
    Q_UNUSED( paintData );


    QSGTexture *texture = nullptr;
    QSGSimpleTextureNode *textureNode = static_cast<QSGSimpleTextureNode *>( node );
    if (!textureNode)
        textureNode = new QSGSimpleTextureNode;

    if ( !renderReady ) {

        // This will be false if the scene graph loads before the core.
        // Which can happen more than you think.

        // If that's the case then just give us a black rectangle.
        simpleTextureNode( Qt::black, textureNode );
        return textureNode;

    }

    else {

        if ( frameDataQueue.length() > 0 ) {
            QImage::Format frame_format = retroToQImageFormat( core->getPixelFormat() );

            FrameData *frame = frameDataQueue.dequeue();

            texture = window()->createTextureFromImage( QImage( frame->data,
                      frame->width,
                      frame->height,
                      frame->pitch,
                      frame_format )
                      , QQuickWindow::TextureOwnsGLTexture );

            delete frame;
        }

        else {
            simpleTextureNode( Qt::black, textureNode );
            return textureNode;
        }

    }

    if( texture == textureNode->texture() ) {

        return textureNode;
    }


    textureNode->setTexture( texture );
    textureNode->setRect( boundingRect() );
    textureNode->setFiltering( QSGTexture::Nearest );
    textureNode->setOwnsTexture( true );
    textureNode->setTextureCoordinatesTransform( QSGSimpleTextureNode::MirrorVertically | QSGSimpleTextureNode::MirrorHorizontally );

    return textureNode;

}

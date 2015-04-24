#ifndef VIDEOITEM_H
#define VIDEOITEM_H

#include <QQuickItem>
#include <QString>
#include <QFile>
#include <QLibrary>
#include <QSGSimpleTextureNode>
#include <QImage>
#include <QOpenGLContext>
#include <QQueue>

#include <QElapsedTimer>


#include <memory>

#include "recorder.h"
#include "core.h"
#include "audio.h"

class VideoItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY( QString libretroCore READ libretroCore WRITE setLibretroCore NOTIFY libretroCoreChanged )
    Q_PROPERTY( QString game READ game WRITE setGame NOTIFY gameChanged )

public:
    VideoItem();
    ~VideoItem();

    QString game() const;
    QString libretroCore() const;

    void setGame(QString game);
    void setLibretroCore(QString libretroCore);

    void simpleTextureNode(Qt::GlobalColor, QSGSimpleTextureNode *textureNode);

signals:
    void libretroCoreChanged();
    void gameChanged();

private slots:
    void handleWindowChanged(QQuickWindow *window);
    void handleOpenGLContextCreated(QOpenGLContext *GLContext);
    void slotHandleFrameData( FrameData *videoFrame );
    void slotHandleAudioData( AudioData *audioFrame );

private:

    QElapsedTimer frameTimer;

    Recorder recorder;

    Audio *audio;
    Core *core;
    bool renderReady;
    bool gameReady;
    bool libretroCoreReady;

    QString qmlLibretroCore;
    QString qmlGame;

    QQueue< FrameData * > frameDataQueue;

    void updateAudioFormat();
    void componentComplete();
    void refresh();

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *paintData);

    static inline QImage::Format retroToQImageFormat( enum retro_pixel_format fmt ) {
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


};

#endif // VIDEOITEM_H

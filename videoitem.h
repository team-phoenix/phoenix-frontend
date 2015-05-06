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

#include "core.h"
#include "audio.h"

class VideoItem : public QQuickItem {
        Q_OBJECT
        Q_PROPERTY( QString libretroCore READ libretroCore WRITE setLibretroCore NOTIFY libretroCoreChanged )
        Q_PROPERTY( QString game READ game WRITE setGame NOTIFY gameChanged )
    public:

        VideoItem();
        ~VideoItem();

        QThread coreThread;

        Core core;
        AudioOutput audioOutput;
        AudioBuffer audioBuffer;

        QTimer coreTimer;

        QElapsedTimer frameTimer;

        bool renderReady;
        bool gameReady;
        bool libretroCoreReady;

        QString qmlLibretroCore;
        QString qmlGame;

        enum Thread {
            CoreThread = 0,
            InputThread,
        };

        QSGTexture *texture;

        QString game() const;
        QString libretroCore() const;

        void setGame( QString game );
        void setLibretroCore( QString libretroCore );

        void startThread( VideoItem::Thread type, QThread::Priority priority = QThread::TimeCriticalPriority );

    signals:

        void libretroCoreChanged();
        void gameChanged();
        void signalDoFrame();

    public slots:

        void handleCoreStateChange( Core::State newState, void *data );

    private slots:

        void handleWindowChanged( QQuickWindow *window );
        void handleOpenGLContextCreated( QOpenGLContext *GLContext );
        void createTexture( uchar *data, unsigned width, unsigned height, int pitch );

    private:

        retro_system_av_info *avInfo;
        retro_pixel_format pixelFormat;

        void componentComplete();
        void refresh();

        void simpleTextureNode( Qt::GlobalColor, QSGSimpleTextureNode *textureNode );

        QSGNode *updatePaintNode( QSGNode *node, UpdatePaintNodeData *paintData );

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

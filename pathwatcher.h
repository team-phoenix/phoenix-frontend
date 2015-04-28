#ifndef PATHWATCHER_H
#define PATHWATCHER_H

#include <QObject>
#include <QUrl>
#include <QStringList>

class PathWatcher : public QObject {
        Q_OBJECT
        QString corePath;
        QStringList coreList;

    public:

        explicit PathWatcher( QObject *parent = 0 );
        ~PathWatcher();

    signals:
        void fileAdded( const QString file, const QString baseName );
        void fileRemoved();

    public slots:
        void slotSetCorePath( const QUrl path );
        void start();
        void clear();

    private slots:
        void slotHandleStarted();

};

#endif // PathWatcher_H

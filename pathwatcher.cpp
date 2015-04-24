#include "pathwatcher.h"

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent>

PathWatcher::PathWatcher( QObject *parent )
    : QObject( parent ),
    corePath( "/usr/local/lib/libretro" )
{
    /*
    QString corePath =
            #ifdef Q_OS_MACX
                "/usr/local/lib/libretro"
            #endif

            #ifdef Q_OS_LINUX
                "/usr/lib/libretro"
            #endif

            #ifdef Q_OS_WIN32
                "C:/libretro"
            #endif
                ;

                */


}

PathWatcher::~PathWatcher()
{


}

void PathWatcher::start()
{
    QFuture< void > future = QtConcurrent::run(this, &PathWatcher::slotHandleStarted);
    Q_UNUSED( future )
}

void PathWatcher::slotSetCorePath( const QUrl path )
{
    corePath = path.toLocalFile();
    start();
}

void PathWatcher::clear()
{
    coreList.clear();
}

void PathWatcher::slotHandleStarted()
{

    QDirIterator dirIter( corePath, QStringList( { "*.so", "*.dylib", "*.dll" } ), QDir::Files, QDirIterator::NoIteratorFlags );

    int fileCount = 0;

    while ( dirIter.hasNext() ) {

        QString file = dirIter.next();

        if ( !coreList.contains( file ) ) {
            emit fileAdded( file, QFileInfo(file).baseName() );
            coreList.append( file );
        }
        fileCount++;

    }

    //if ( fileCount != coreList.size() )
      //  emit fileRemoved();

}


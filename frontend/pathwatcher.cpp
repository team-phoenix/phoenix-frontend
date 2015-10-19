#include "pathwatcher.h"

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent>

PathWatcher::PathWatcher( QObject *parent )
    : QObject( parent ) {

#ifdef Q_OS_MACX
    corePath = "/usr/local/lib/libretro";
#endif

#ifdef Q_OS_LINUX
    corePath = "/usr/lib/libretro";
#endif

#ifdef Q_OS_WIN32
    corePath = "C:/Program Files/Libretro/Cores";
#endif

}

PathWatcher::~PathWatcher() {


}

void PathWatcher::start() {
    slotHandleStarted();
}

void PathWatcher::slotSetCorePath( const QUrl path ) {
    corePath = path.toLocalFile();
    start();
}

void PathWatcher::clear() {
    coreList.clear();
}

void PathWatcher::slotHandleStarted() {

    QDirIterator dirIter( corePath, QStringList( { "*.so", "*.dylib", "*.dll" } ),
                          QDir::Files, QDirIterator::NoIteratorFlags );

    int fileCount = 0;

    while( dirIter.hasNext() ) {

        QString file = QUrl::fromLocalFile( dirIter.next() ).toString().remove( "file://" );

        if( !coreList.contains( file ) ) {
            emit fileAdded( file, QFileInfo( file ).baseName() );
            coreList.append( file );
        }

        fileCount++;

    }

    //if ( fileCount != coreList.size() )
    //  emit fileRemoved();

}


#include <QApplication>
#include <QtCore>
#include <QQmlApplicationEngine>

#include "videoitem.h"
#include "core.h"
#include "pathwatcher.h"
#include "libretro.h"

Q_DECLARE_METATYPE( retro_system_av_info )
Q_DECLARE_METATYPE( retro_pixel_format )
Q_DECLARE_METATYPE( Core::State )
Q_DECLARE_METATYPE( Core::Error )

int main( int argc, char *argv[] ) {
    QApplication app( argc, argv );

    QQmlApplicationEngine engine;

    QObject::connect( &engine, &QQmlApplicationEngine::quit, &app, &QApplication::quit );

    qmlRegisterType<VideoItem>( "libretro.video", 1, 0, "VideoItem" );
    qmlRegisterType<PathWatcher>( "libretro.video", 1, 0, "PathWatcher" );
    qRegisterMetaType<retro_system_av_info>();
    qRegisterMetaType<retro_pixel_format>();
    qRegisterMetaType<Core::State>();
    qRegisterMetaType<Core::Error>();

    engine.load( QUrl( QString( "qrc:/main.qml" ) ) );

    return app.exec();
}

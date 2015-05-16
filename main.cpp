#include <QtGlobal>
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

void phoenixDebugMessageHandler( QtMsgType type, const QMessageLogContext &context, const QString &msg ) {

    // Change this QString to reflect the message you want to get a stack trace for
    if( QString( msg ).contains( "Timers cannot be stopped from another thread" ) ) {

        int breakPointOnThisLine( 0 );
        Q_UNUSED( breakPointOnThisLine );

    }

    QByteArray localMsg = msg.toLocal8Bit();

    switch( type ) {
        case QtDebugMsg:
            fprintf( stderr, "Debug: %s (%s:%u, %s)\n",
                     localMsg.constData(), context.file, context.line, context.function );
            break;

        case QtWarningMsg:
            fprintf( stderr, "Warning: %s (%s:%u, %s)\n",
                     localMsg.constData(), context.file, context.line, context.function );
            break;

        case QtCriticalMsg:
            fprintf( stderr, "Critical: %s (%s:%u, %s)\n",
                     localMsg.constData(), context.file, context.line, context.function );
            break;

        case QtFatalMsg:
            fprintf( stderr, "Fatal: %s (%s:%u, %s)\n",
                     localMsg.constData(), context.file, context.line, context.function );
            abort();
    }

}

int main( int argc, char *argv[] ) {

    // qInstallMessageHandler( phoenixDebugMessageHandler );

    QApplication app( argc, argv );

    QQmlApplicationEngine engine;

    // Necessary to quit properly
    QObject::connect( &engine, &QQmlApplicationEngine::quit, &app, &QApplication::quit );

    // Make C++ classes visible to QML
    qmlRegisterType<VideoItem>( "libretro.video", 1, 0, "VideoItem" );
    qmlRegisterType<PathWatcher>( "libretro.video", 1, 0, "PathWatcher" );

    // Don't let the Qt police find out we're declaring these structs as metatypes
    // without proper constructors/destructors declared/written
    qRegisterMetaType<retro_system_av_info>();
    qRegisterMetaType<retro_pixel_format>();
    qRegisterMetaType<Core::State>();
    qRegisterMetaType<Core::Error>();

    engine.load( QUrl( QString( "qrc:/main.qml" ) ) );

    return app.exec();

}

#include <QtGlobal>
#include <QApplication>
#include <QtCore>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "videoitem.h"
#include "pathwatcher.h"

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
            break;
        default:
            Q_UNREACHABLE();
            break;
    }

}

int main( int argc, char *argv[] ) {

    // qInstallMessageHandler( phoenixDebugMessageHandler );

    QApplication app( argc, argv );

    QApplication::setApplicationDisplayName( "Coatl" );
    QApplication::setApplicationName( "Coatal" );
    QApplication::setApplicationVersion( "1.0" );
    QApplication::setOrganizationDomain( "http://phoenix.vg/" );

    QQmlApplicationEngine engine;

    // Necessary to quit properly
    //QObject::connect( &engine, &QQmlApplicationEngine::quit, &app, &QApplication::quit );

    // Make C++ classes visible to QML
    VideoItem::registerTypes();
    InputManager::registerTypes();
    qmlRegisterType<PathWatcher>( "paths", 1, 0, "PathWatcher" );


    engine.load( QUrl( QStringLiteral( "qrc:/main.qml" ) ) );

    return app.exec();

}

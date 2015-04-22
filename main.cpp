#include <QApplication>
#include <QQmlApplicationEngine>

#include "videoitem.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QQmlApplicationEngine engine;

    qmlRegisterType<VideoItem>("libretro.video", 1, 0, "VideoItem");
    engine.load(QUrl(QString("qrc:/main.qml")));

    return app.exec();
}

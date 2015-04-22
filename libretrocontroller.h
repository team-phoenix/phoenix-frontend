#ifndef LIBRETROCONTROLLER_H
#define LIBRETROCONTROLLER_H

#include <QQuickItem>

#include "core.h"
#include "audio.h"
#include "videoitem.h"

class LibretroController : public QQuickItem
{
    Q_OBJECT
public:
    LibretroController();
    ~LibretroController();

signals:

public slots:

private:
    Core core;
    Audio audio;
    VideoItem videoItem;

};

#endif // LIBRETROCONTROLLER_H

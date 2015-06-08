TEMPLATE = app

QT += qml quick widgets multimedia concurrent

HEADERS += \
    videoitem.h \
    core.h \
    libretro.h \
    logging.h \
    audiobuffer.h \
    phoenixglobals.h \
    pathwatcher.h \
    audiooutput.h \
    input/keyboard.h \
    input/inputmanager.h \
    input/inputdevice.h \
    input/inputdeviceevent.h \
    input/joystick.h \
    input/sdleventloop.h

SOURCES += main.cpp \
    videoitem.cpp \
    core.cpp \
    audiobuffer.cpp \
    phoenixglobals.cpp \
    pathwatcher.cpp \
    audiooutput.cpp \
    logging.cpp \
    input/keyboard.cpp \
    input/inputmanager.cpp \
    input/joystick.cpp \
    input/sdleventloop.cpp \
    input/inputdevice.cpp \
    input/inputdeviceevent.cpp

RESOURCES += qml.qrc

LIBS += -lsamplerate

macx {
    LIBS += -lSDL2
    INCLUDEPATH += /usr/local/include /usr/local/include/SDL2
    QMAKE_CXXFLAGS +=
    QMAKE_LFLAGS += -L/usr/local/lib
}

win32 {
    CONFIG -= windows
    QMAKE_LFLAGS += $$QMAKE_LFLAGS_WINDOWS

    LIBS += -LC:/SDL2/lib
    LIBS += -lmingw32 -lSDL2main -lSDL2 -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid

    DEFINES += SDL_WIN
    INCLUDEPATH += C:/SDL2/include C:/msys64/mingw64/include/SDL2 C:/msys64/mingw32/include/SDL2


    CONFIG(debug, debug|release)  {
        depends.path = $$OUT_PWD/debug
        depends.files += C:/SDL2/bin/SDL2.dll
    }

    CONFIG(release, debug|release) {
        depends.path = $$OUT_PWD/release
        depends.files += C:/SDL2/bin/SDL2.dll
    }
    INSTALLS += depends

}

else {
    LIBS += -lSDL2
}


CONFIG += c++11

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)


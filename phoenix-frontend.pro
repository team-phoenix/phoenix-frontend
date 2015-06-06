TEMPLATE = app

QT += qml quick widgets multimedia concurrent

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
    LIBS += -framework SDL2
    INCLUDEPATH += /Library/Frameworks/SDL2.framework/Headers /usr/local/include
    QMAKE_CXXFLAGS += -F/Library/Frameworks
    QMAKE_LFLAGS += -F/Library/Frameworks -L/usr/local/lib
}

else {
    LIBS += -lSDL2
}


CONFIG += c++11

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

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

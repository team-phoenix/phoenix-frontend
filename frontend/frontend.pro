TEMPLATE = app

TARGET = Coatl

QT += qml quick widgets multimedia concurrent

##
## Compiler settings
##

    CONFIG += c++11

    OBJECTS_DIR = obj
    MOC_DIR     = moc
    RCC_DIR     = rcc
    UI_DIR      = gui

    # FIXME: Remove once newer Qt versions make this unnecessary
    macx: QMAKE_MAC_SDK = macosx10.11

    # SDL 2
    # http://web.archive.org/web/20150305002626/http://blog.debao.me/2013/07/link-confilict-between-sdl-and-qt-under-windows/
    # Applies to both compiler and linker stages
    win32: CONFIG -= windows
    win32: QMAKE_LFLAGS += $$QMAKE_LFLAGS_WINDOWS

    # Include libraries
    win32: INCLUDEPATH += C:/msys64/mingw64/include C:/msys64/mingw64/include/SDL2 # MSYS2
    macx:  INCLUDEPATH += /usr/local/include /usr/local/include/SDL2               # Homebrew
    macx:  INCLUDEPATH += /usr/local/include /opt/local/include/SDL2               # MacPorts
    unix:  INCLUDEPATH += /usr/include /usr/include/SDL2                           # Linux

INCLUDEPATH += ../backend ../backend/input

HEADERS += pathwatcher.h


SOURCES += main.cpp \
           pathwatcher.cpp


RESOURCES += qml.qrc




##
## Linker settings
##

    ##
    ## Library paths
    ##

    # Our stuff
    LIBS += -L../backend

    # Force the Phoenix binary to be relinked if the backend code has changed
    TARGETDEPS += ../backend/libphoenix-backend.a

    # SDL2
    macx: LIBS += -L/usr/local/lib -L/opt/local/lib # Homebrew, MacPorts

    ##
    ## Libraries
    ##

    # Our stuff
    LIBS += -lphoenix-backend

    # SDL 2
    win32: LIBS += -lmingw32 -lSDL2main
    LIBS += -lSDL2

    # Other libraries we use
    LIBS += -lsamplerate -lz

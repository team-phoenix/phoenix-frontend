#ifndef SDLEVENTLOOP_H
#define SDLEVENTLOOP_H

#include <QObject>
#include <QTimer>
#include <SDL.h>
#include <QList>
#include <QMutex>

#include <QThread>
#include "joystick.h"

class SDLEventLoop : public QObject
{
    Q_OBJECT
    QTimer sdlPollTimer;
    int numOfDevices;
    QMutex mutex;

    QThread sdlEventLoopThread;

    // The InputManager is in charge of deleting these devices.
    // The InputManager gains access to these devices by the
    // deviceConnected( Joystick * ) signal.

    // Also for this list, make use the 'which' index, for
    // propery insertions and retrievals.
    QList<Joystick *> sdlDeviceList;

public:
    explicit SDLEventLoop( QObject *parent = 0 );

public slots:
    void processEvents();

    void start()
    {
        sdlEventLoopThread.start( QThread::HighPriority );
    }

    void startTimer()
    {
        findJoysticks();
        sdlPollTimer.start();
    }

    void stop()
    {
        sdlEventLoopThread.terminate();
        sdlEventLoopThread.wait();
    }

signals:
    void deviceConnected( Joystick * );
    void deviceRemoved( Joystick * );

private:
    void initSDL();

    void findJoysticks();

    void quitSDL();

};

#endif // SDLEVENTLOOP_H

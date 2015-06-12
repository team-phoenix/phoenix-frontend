#ifndef SDLEVENTLOOP_H
#define SDLEVENTLOOP_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QHash>
#include <SDL.h>

#include "joystick.h"

class SDLEventLoop : public QObject {
        Q_OBJECT
        QTimer sdlPollTimer;
        int numOfDevices;
        QMutex sdlEventMutex;

        bool forceEventsHandling;

        // The InputManager is in charge of deleting these devices.
        // The InputManager gains access to these devices by the
        // deviceConnected( Joystick * ) signal.

        // Also for this list, make use the 'which' index, for
        // propery insertions and retrievals.
        QList<Joystick *> sdlDeviceList;
        QHash<int, int> deviceLocationMap;

    public:
        explicit SDLEventLoop( QObject *parent = 0 );

    public slots:
        void processEvents();

        void start() {
            sdlPollTimer.start();
        }

        void startTimer() {
        }

        void stop() {
            sdlPollTimer.stop();
        }

    signals:
        void deviceConnected( Joystick *joystick );
        void deviceRemoved( int which );

    private:
        void initSDL();
        void buttonChanged( SDL_ControllerButtonEvent &controllerEvent );

        void findJoysticks();

        void quitSDL();


};

#endif // SDLEVENTLOOP_H

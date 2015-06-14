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

        void quitSDL();

        int16_t getControllerState( Joystick *joystick, const SDL_GameControllerButton &button )
        {
            int value = joystick->sdlButtonMapping().value( button, -1 );
            if ( value == -1 ) {
                value = joystick->sdlAxisMapping().value( button, -1 );
                return SDL_JoystickGetAxis( joystick->sdlJoystick(), value );
            }

            return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
        }

        int16_t getControllerState( Joystick *joystick, const SDL_GameControllerAxis &axis )
        {
            int value = joystick->sdlAxisMapping().value( axis, -1 );
            if ( value == -1 ) {
                value = joystick->sdlButtonMapping().value( axis, -1 );
                return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
            }


            //qDebug() << axis << value;
            if ( value >= SDL_CONTROLLER_AXIS_MAX )
                return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
            return SDL_JoystickGetAxis( joystick->sdlJoystick(), value );
        }


};

#endif // SDLEVENTLOOP_H

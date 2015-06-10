#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <QMap>

#include "input/inputdevice.h"
#include "libretro.h"
#include "SDL.h"

class Joystick : public InputDevice {

    public:
        // This is needed to stop a complication warning in the reimplemented insert function.
        using InputDevice::insert;

    static const int maxNumOfDevices;

        enum SDLType {
            SDLGamepad = 0,
            SDLJoystick,
        };

        explicit Joystick( const int joystickIndex, QObject *parent = 0 );
        ~Joystick();

        SDLType sdlType() const {
            return qmlSdlType;
        }

        QString guid() const;

        int buttonCount() const;

        int ballCount() const;

        int hatCount() const;

        int axisCount() const;

        int sdlIndex() const;

        qreal deadZone() const;

        SDL_GameController *sdlDevice() const;

        SDL_JoystickID instanceID() const;

        void setSDLIndex( const int index );

        void close();

    public slots:

        void insert( const quint8 &event, const int16_t pressed );

    private:

        QString qmlGuid;
        int qmlInstanceID;
        int qmlSdlIndex;
        int qmlButtonCount;
        int qmlAxisCount;
        int qmlHatCount;
        int qmlBallCount;
        qreal qmlDeadZone;

        SDLType qmlSdlType;

        SDL_GameController *device;

};

#endif // JOYSTICK_H

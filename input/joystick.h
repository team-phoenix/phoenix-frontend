#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <QMap>

#include "input/inputdevice.h"
#include "libretro.h"
#include "SDL.h"

class Joystick : public InputDevice
{

public:
    // This is needed to stop a complication warning in the reimplemented insert function.
    using InputDevice::insert;

    enum SDLType {
        SDLGamepad = 0,
        SDLJoystick,
    };

    explicit Joystick( const int joystickIndex, QObject* parent = 0 );
    ~Joystick();

    SDLType sdlType() const
    {
        return qmlSdlType;
    }

    QString guid() const;

    int buttonCount() const;

    int ballCount() const;

    int hatCount() const;

    int axisCount() const;

    qreal deadZone() const;

    SDL_Joystick *sdlDevice() const;

public slots:

    void insert( const quint8 &event, const int16_t pressed );

private:

    QString qmlGuid;
    int qmlButtonCount;
    int qmlAxisCount;
    int qmlHatCount;
    int qmlBallCount;
    qreal qmlDeadZone;

    SDLType qmlSdlType;

    SDL_Joystick *device;

    void populateDeviceMapping();

};

#endif // JOYSTICK_H

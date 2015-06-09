#include "joystick.h"

const int Joystick::maxNumOfDevices = 4;

Joystick::Joystick( const int joystickIndex, QObject *parent )
    : InputDevice( LibretroType::RetroGamepad, parent ),


      qmlDeadZone( 0 )

{
    device = SDL_GameControllerOpen( joystickIndex );
    setName( SDL_GameControllerName( device ) );
/*
    qmlAxisCount = SDL_JoystickNumAxes( device );
    qmlButtonCount = SDL_JoystickNumButtons( device );

    qmlHatCount = SDL_JoystickNumHats( device );
    qmlBallCount = SDL_JoystickNumBalls( device );

    char guidStr[1024];
    SDL_JoystickGUID guid = SDL_JoystickGetGUID( device );
    SDL_JoystickGetGUIDString( guid, guidStr, sizeof( guidStr ) );

    qmlGuid = guidStr;

    if( QByteArray( SDL_GameControllerMappingForGUID( guid ) ).isEmpty() ) {
        qmlSdlType = SDLType::SDLJoystick;

        //populateDeviceMapping();

    } else {
        qmlSdlType = SDLType::SDLGamepad;
    }
*/

}

Joystick::~Joystick() {
    Q_ASSERT_X( device, "InputDevice" , "the device was deleted by an external source" );
    SDL_GameControllerClose( device );
}

QString Joystick::guid() const {
    return qmlGuid;
}

int Joystick::buttonCount() const {
    return qmlButtonCount;
}

int Joystick::ballCount() const {
    return qmlBallCount;
}

int Joystick::hatCount() const {
    return qmlHatCount;
}

int Joystick::axisCount() const {
    return qmlAxisCount;
}

qreal Joystick::deadZone() const {
    return qmlDeadZone;
}

SDL_GameController *Joystick::sdlDevice() const {
    return device;
}

void Joystick::insert( const quint8 &event, const int16_t pressed ) {
    InputDevice::insert( new InputDeviceEvent( event, pressed, this ) );
}

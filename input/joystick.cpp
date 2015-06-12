#include "joystick.h"

const int Joystick::maxNumOfDevices = 128;

Joystick::Joystick( const int joystickIndex, QObject *parent )
    : InputDevice( LibretroType::RetroGamepad, parent ),


      qmlSdlIndex( joystickIndex ),
      qmlDeadZone( 12000 ),
      qmlAnalogMode( false )


{
    device = SDL_GameControllerOpen( joystickIndex );
    setName( SDL_GameControllerName( device ) );
    qmlInstanceID = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( device ) );
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
    close();
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

int Joystick::sdlIndex() const
{
    return qmlSdlIndex;
}

qreal Joystick::deadZone() const {
    return qmlDeadZone;
}

bool Joystick::analogMode() const
{
    return qmlAnalogMode;
}

void Joystick::setAnalogMode(const bool mode)
{
    qmlAnalogMode = mode;
}

SDL_GameController *Joystick::sdlDevice() const {
    return device;
}

SDL_JoystickID Joystick::instanceID() const
{
    return qmlInstanceID;
}

void Joystick::setSDLIndex(const int index)
{
    qmlSdlIndex = index;
}

void Joystick::close()
{
    Q_ASSERT_X( device, "InputDevice" , "the device was deleted by an external source" );
    SDL_GameControllerClose( device );
}

void Joystick::insert( const quint8 &event, const int16_t pressed ) {
    InputDevice::insert( new InputDeviceEvent( event, pressed, this ) );
}

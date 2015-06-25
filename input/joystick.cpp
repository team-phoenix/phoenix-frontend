#include "joystick.h"

const int Joystick::maxNumOfDevices = 128;

Joystick::Joystick( const int joystickIndex, QObject *parent )
    : InputDevice( LibretroType::DigitalGamepad, parent ),
      qmlSdlIndex( joystickIndex ),
      qmlDeadZone( 12000 ),
      qmlAnalogMode( false ) {
    device = SDL_GameControllerOpen( joystickIndex );
    setName( SDL_GameControllerName( device ) );
    qmlInstanceID = SDL_JoystickInstanceID( SDL_GameControllerGetJoystick( device ) );

    auto *sdlJoystick = SDL_GameControllerGetJoystick( device );
    qmlAxisCount = SDL_JoystickNumAxes( sdlJoystick );
    qmlButtonCount = SDL_JoystickNumButtons( sdlJoystick );

    qmlHatCount = SDL_JoystickNumHats( sdlJoystick );
    qmlBallCount = SDL_JoystickNumBalls( sdlJoystick );

    char guidStr[1024];
    SDL_JoystickGUID guid = SDL_JoystickGetGUID( sdlJoystick );
    SDL_JoystickGetGUIDString( guid, guidStr, sizeof( guidStr ) );

    qmlGuid = guidStr;

    // This is really annoying, but for whatever reason, the SDL2 Game Controller API,
    // doesn't assign a proper mapping value certain controller buttons.
    // This means that we have to hold the mapping ourselves and do it correctly.

    populateMappings( device );



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

int Joystick::sdlIndex() const {
    return qmlSdlIndex;
}

qreal Joystick::deadZone() const {
    return qmlDeadZone;
}

bool Joystick::analogMode() const {
    return qmlAnalogMode;
}

QHash<QString, int> &Joystick::sdlMapping() {
    return sdlControllerMapping;
}

void Joystick::setAnalogMode( const bool mode ) {
    qmlAnalogMode = mode;
}

SDL_GameController *Joystick::sdlDevice() const {
    return device;
}

SDL_Joystick *Joystick::sdlJoystick() const {
    return SDL_GameControllerGetJoystick( device );
}

SDL_JoystickID Joystick::instanceID() const {
    return qmlInstanceID;
}

void Joystick::setSDLIndex( const int index ) {
    qmlSdlIndex = index;
}

void Joystick::close() {
    Q_ASSERT_X( device, "InputDevice" , "the device was deleted by an external source" );
    SDL_GameControllerClose( device );
}

void Joystick::setMapping( const QVariantMap mapping ) {
    QString platform;
#if defined(Q_OS_OSX)
    platform = "Mac OS X";
#elif defined(Q_OS_WIN32)
    platform = "Windows";
#elif defined(Q_OS_LINUX)
    platform = "Linux";
#else
#error "Your operating system does not support this feature. ( Joystick::mappingString() )"
#endif
    const QString prefix = guid() + "," + name();
    const QString suffix = "platform:" + platform;

    QString body;

    for( auto &key : mapping.keys() ) {
        body += key + ":" + mapping.value( key ).toString() + ",";
    }

    qmlMappingString = prefix + "," + body + suffix + ",";

}

void Joystick::populateMappings( SDL_GameController *device ) {
    // Handle populating our own mappings, because SDL2 often uses the incorrect mapping array.
    QString mappingString = SDL_GameControllerMapping( device );

    auto strList = mappingString.split( "," );

    for( QString &str : strList ) {
        auto keyValuePair = str.split( ":" );

        if( keyValuePair.size() <= 1 ) {
            continue;
        }

        auto key = keyValuePair.at( 0 );
        auto value = keyValuePair.at( 1 );

        if( value.isEmpty() ) {
            qCWarning( phxInput ) << "The value for " << key << " is empty.";
            continue;
        }

        if ( key == "platform" )
            continue;


        int numberValue = value.remove( value.at( 0 ) ).toInt();

        auto newEvent = InputDeviceEvent::Unknown;
        if( key == "a" ) {
            newEvent = InputDeviceEvent::A;
        } else if( key == "b" ) {
            newEvent = InputDeviceEvent::B;
        } else if( key == "x" ) {
            newEvent = InputDeviceEvent::X;
        } else if( key == "y" ) {
            newEvent = InputDeviceEvent::Y;
        } else if( key == "back" ) {
            newEvent = InputDeviceEvent::Select;
        } else if( key == "start" ) {
            newEvent = InputDeviceEvent::Start;
        } else if( key == "guide" ) {
            newEvent = InputDeviceEvent::Unknown;
        } else if( key == "leftstick" ) {
            newEvent = InputDeviceEvent::L3;
        } else if( key == "rightstick" ) {
            newEvent = InputDeviceEvent::R3;
        } else if( key == "leftshoulder" ) {
            newEvent = InputDeviceEvent::L;
        } else if( key == "rightshoulder" ) {
            newEvent = InputDeviceEvent::R;
        } else if( key == "dpup" ) {
            newEvent = InputDeviceEvent::Up;
        } else if( key == "dpdown" ) {
            newEvent = InputDeviceEvent::Down;
        } else if( key == "dpleft" ) {
            newEvent = InputDeviceEvent::Left;
        } else if( key == "dpright" ) {
            newEvent = InputDeviceEvent::Right;
        } else if( key == "lefttrigger" ) {
            newEvent = InputDeviceEvent::L2;
        } else if( key == "righttrigger" ) {
            newEvent = InputDeviceEvent::R2;
        } else if( key == "leftx"
               || key == "lefty"
               || key == "rightx"
               || key == "righty"
               || key == "lefttrigger"
               || key == "righttrigger") {
            newEvent = InputDeviceEvent::Axis;
        } else {
            qCWarning( phxInput ) << key << "was missed in the mapping.";
        }

        if ( newEvent != InputDeviceEvent::Unknown ) {
            mapping().insert( key, newEvent );
            sdlControllerMapping.insert( key, numberValue );
        }
    }
}

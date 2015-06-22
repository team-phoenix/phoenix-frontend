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

QHash<int, int> &Joystick::sdlButtonMapping() {
    return buttonMapping;
}

QHash<int, int> &Joystick::sdlAxisMapping() {
    return axisMapping;
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

        // Handle adding buttons to map;
        if( value.at( 0 ) == 'b' ) {
            value = value.remove( "b" );

            int numVal = value.toInt();

            if( key == "a" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_A, numVal );
            } else if( key == "b" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_B, numVal );
            } else if( key == "x" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_X, numVal );
            } else if( key == "y" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_Y, numVal );
            } else if( key == "back" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_BACK, numVal );
            } else if( key == "start" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_START, numVal );
            } else if( key == "guide" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_GUIDE, numVal );
            } else if( key == "leftstick" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_LEFTSTICK, numVal );
            } else if( key == "rightstick" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_RIGHTSTICK, numVal );
            } else if( key == "leftshoulder" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_LEFTSHOULDER, numVal );
            } else if( key == "rightshoulder" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, numVal );
            } else if( key == "dpup" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_DPAD_UP, numVal );
            } else if( key == "dpdown" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_DPAD_DOWN, numVal );
            } else if( key == "dpleft" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_DPAD_LEFT, numVal );
            } else if( key == "dpright" ) {
                buttonMapping.insert( SDL_CONTROLLER_BUTTON_DPAD_RIGHT, numVal );
            } else if( key == "lefttrigger" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_TRIGGERLEFT, numVal );
            } else if( key == "righttrigger" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_TRIGGERRIGHT, numVal );
            } else {
                qCWarning( phxInput ) << numVal <<  "was missed in the mapping.";
            }
        }
        // Handle adding axis values into map.
        else if( value.at( 0 ) == 'a' ) {
            value = value.remove( "a" );
            int numVal = value.toInt();

            if( key == "leftx" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_LEFTX, numVal );
            } else if( key == "lefty" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_LEFTY, numVal );
            } else if( key == "rightx" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_RIGHTX, numVal );
            } else if( key == "righty" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_RIGHTY, numVal );
            } else if( key == "lefttrigger" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_TRIGGERLEFT, numVal );
            } else if( key == "righttrigger" ) {
                axisMapping.insert( SDL_CONTROLLER_AXIS_TRIGGERRIGHT, numVal );
            } else {
                qCWarning( phxInput ) << numVal << "was missed in the mapping.";
            }
        }
    }

}

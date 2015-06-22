#include "sdleventloop.h"

#include "logging.h"

#include <QFile>
#include <QMutexLocker>


SDLEventLoop::SDLEventLoop( QObject *parent )
    : QObject( parent ),
      sdlPollTimer( this ),
      numOfDevices( 0 ),
      forceEventsHandling( true ) {
    // To do the poll timer isn't in the sdlEventLoopThread. it needs to be.

    // Ensures the resources at loaded at startup, even during
    // static compilation.
    Q_INIT_RESOURCE( controllerdb );
    QFile file( ":/input/gamecontrollerdb.txt" );
    Q_ASSERT( file.open( QIODevice::ReadOnly ) );

    auto mappingData = file.readAll();

    SDL_SetHint( SDL_HINT_GAMECONTROLLERCONFIG, mappingData.constData() );

    file.close();


    for( int i = 0; i < Joystick::maxNumOfDevices; ++i ) {
        sdlDeviceList.append( nullptr );
    }


    sdlPollTimer.setInterval( 5 );

    connect( &sdlPollTimer, &QTimer::timeout, this, &SDLEventLoop::pollEvents );

    // Load SDL
    initSDL();

}

void SDLEventLoop::pollEvents() {

    if( !forceEventsHandling ) {

        // Update all connected controller states.
        SDL_JoystickUpdate();

        // All joystick instance ID's are stored inside of this map.
        // This is necessary because the instance ID could be any number, and
        // so cannot be used for indexing the deviceLocationMap. The value of the map
        // is the actual index that the sdlDeviceList uses.
        for( auto &key : deviceLocationMap.keys() ) {
            auto index = deviceLocationMap[ key ];

            auto *joystick = sdlDeviceList.at( index );
            auto *sdlGamepad = joystick->sdlDevice();

            // Check to see if sdlGamepad is actually connected. If it isn't this will terminate the
            // polling and initialize the event handling.

            forceEventsHandling = SDL_GameControllerGetAttached( sdlGamepad ) == SDL_FALSE;

            if( forceEventsHandling ) {
                return;
            }

            // Get all button states
            bool up = getControllerState( joystick,
                                          SDL_CONTROLLER_BUTTON_DPAD_UP );
            bool down = getControllerState( joystick,
                                            SDL_CONTROLLER_BUTTON_DPAD_DOWN );
            bool left = getControllerState( joystick,
                                            SDL_CONTROLLER_BUTTON_DPAD_LEFT );
            bool right = getControllerState( joystick,
                                             SDL_CONTROLLER_BUTTON_DPAD_RIGHT );
            bool start = getControllerState( joystick,
                                             SDL_CONTROLLER_BUTTON_START );
            bool back = getControllerState( joystick,
                                            SDL_CONTROLLER_BUTTON_BACK );
            bool b = getControllerState( joystick,
                                         SDL_CONTROLLER_BUTTON_B );
            bool a = getControllerState( joystick,
                                         SDL_CONTROLLER_BUTTON_A );
            bool x = getControllerState( joystick,
                                         SDL_CONTROLLER_BUTTON_X );
            bool y = getControllerState( joystick,
                                         SDL_CONTROLLER_BUTTON_Y );
            bool leftShoulder = getControllerState( joystick,
                                                    SDL_CONTROLLER_BUTTON_LEFTSHOULDER );
            bool rightShoulder = getControllerState( joystick,
                                 SDL_CONTROLLER_BUTTON_RIGHTSHOULDER );
            bool leftStick = getControllerState( joystick,
                                                 SDL_CONTROLLER_BUTTON_LEFTSTICK );
            bool rightStick = getControllerState( joystick,
                                                  SDL_CONTROLLER_BUTTON_RIGHTSTICK );

            // Insert all button states except for directional buttons
            joystick->insert( InputDeviceEvent::Start, start );
            joystick->insert( InputDeviceEvent::Select, back );
            joystick->insert( InputDeviceEvent::B, a );
            joystick->insert( InputDeviceEvent::A, b );
            joystick->insert( InputDeviceEvent::X, y );
            joystick->insert( InputDeviceEvent::Y, x );
            joystick->insert( InputDeviceEvent::L, leftShoulder );
            joystick->insert( InputDeviceEvent::R, rightShoulder );
            joystick->insert( InputDeviceEvent::L3, leftStick );
            joystick->insert( InputDeviceEvent::R3, rightStick );


            int16_t leftXAxis = getControllerState( joystick,
                                                    SDL_CONTROLLER_AXIS_LEFTX );

            int16_t leftYAxis = getControllerState( joystick,
                                                    SDL_CONTROLLER_AXIS_LEFTY );

            int16_t rightXAxis = getControllerState( joystick,
                                 SDL_CONTROLLER_AXIS_RIGHTX );

            int16_t rightYAxis = getControllerState( joystick,
                                 SDL_CONTROLLER_AXIS_RIGHTY );

            int16_t leftTrigger = getControllerState( joystick,
                                  SDL_CONTROLLER_AXIS_TRIGGERLEFT );
            int16_t rightTrigger = getControllerState( joystick,
                                   SDL_CONTROLLER_AXIS_TRIGGERRIGHT );

            // Insert the trigger buttons.
            joystick->insert( InputDeviceEvent::L2, leftTrigger );
            joystick->insert( InputDeviceEvent::R2, rightTrigger );

            Q_UNUSED( rightXAxis );
            Q_UNUSED( rightYAxis );

            if( !joystick->analogMode() ) {

                // The directional buttons need to be assigned again, so that
                // the left analog stick can be used as a D-PAD.
                if( !left && leftXAxis <= 0 ) {
                    left = ( leftXAxis < -joystick->deadZone() );
                }

                if( !down && leftXAxis > 0 ) {
                    right = ( leftXAxis > joystick->deadZone() );
                }

                if( !up && leftYAxis <= 0 ) {
                    up = ( leftYAxis < -joystick->deadZone() );
                }

                if( !down && leftYAxis > 0 ) {
                    down = ( leftYAxis > joystick->deadZone() );
                }
            }

            else {
                // Not handled yet.
                // analogMode() will be true, when we support libretro's analog controller API.
            }

            // Insert directional buttons.
            joystick->insert( InputDeviceEvent::Up, up );
            joystick->insert( InputDeviceEvent::Down, down );
            joystick->insert( InputDeviceEvent::Left, left );
            joystick->insert( InputDeviceEvent::Right, right );

        }
    }

    else {
        SDL_Event sdlEvent;

        // The only events that should be handled here are, SDL_CONTROLLERDEVICEADDED
        // and SDL_CONTROLLERDEVICEREMOVED.
        while( SDL_PollEvent( &sdlEvent ) ) {

            switch( sdlEvent.type ) {

                case SDL_CONTROLLERDEVICEADDED: {
                    forceEventsHandling = false;

                    // This needs to be checked for, because the first time a controller
                    // sdl starts up, it fires this signal twice, pretty annoying...

                    if( sdlDeviceList.at( sdlEvent.cdevice.which ) != nullptr ) {
                        qCDebug( phxInput ).nospace() << "Duplicate controller added at slot "
                                                      << sdlEvent.cdevice.which << ", ignored";
                        break;
                    }

                    auto *joystick = new Joystick( sdlEvent.cdevice.which );

                    deviceLocationMap.insert( joystick->instanceID(), sdlEvent.cdevice.which );

                    sdlDeviceList[ sdlEvent.cdevice.which ] = joystick;

                    emit deviceConnected( joystick );

                    break;
                }

                case SDL_CONTROLLERDEVICEREMOVED: {

                    int index = deviceLocationMap.value( sdlEvent.cbutton.which, -1 );

                    Q_ASSERT( index != -1 );

                    auto *joystick = sdlDeviceList.at( index );

                    Q_ASSERT( joystick != nullptr );

                    if( joystick->instanceID() == sdlEvent.cdevice.which ) {
                        emit deviceRemoved( joystick->sdlIndex() );
                        sdlDeviceList[ index ] = nullptr;
                        deviceLocationMap.remove( sdlEvent.cbutton.which );
                        forceEventsHandling = true;
                        break;
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }
}

void SDLEventLoop::start() {
    sdlPollTimer.start();
}

void SDLEventLoop::stop() {
    sdlPollTimer.stop();
}

void SDLEventLoop::initSDL() {

    if( SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 ) {
        qFatal( "Fatal: Unable to initialize SDL2: %s", SDL_GetError() );
    }

    // Allow game controller event states to be automatically updated.
    SDL_GameControllerEventState( SDL_ENABLE );
}

void SDLEventLoop::quitSDL() {
    SDL_Quit();
}

int16_t SDLEventLoop::getControllerState(Joystick *joystick, const SDL_GameControllerButton &button) {
    // This first checks to see if a button located in the sdlButtonMapping().
    int value = joystick->sdlButtonMapping().value( button, -1 );

    // If the value is not in the sdlButtonMapping(), look for it in the
    // sdlAxisMapping.
    if( value == -1 ) {
        value = joystick->sdlAxisMapping().value( button, -1 );
        return SDL_JoystickGetAxis( joystick->sdlJoystick(), value );
    }

    return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
}

int16_t SDLEventLoop::getControllerState(Joystick *joystick, const SDL_GameControllerAxis &axis) {
    int value = joystick->sdlAxisMapping().value( axis, -1 );

    // First check to see if the button is located in sdlAxisMapping().

    // If the value is not in the sdlAxisMapping(), look for the axis in
    // the sdlButtonMapping(). This can happen with the Wii U Pro Controller
    // because the Wii U Pro Controller has digital trigger buttons, instead of analog triggers.
    if ( value == -1 ) {
        value = joystick->sdlButtonMapping().value( axis, -1 );
        return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
    }

    // If the returned value is greater than SDL_CONTROLLER_AXIS_MAX, then this value
    // is a button.
    if ( value >= SDL_CONTROLLER_AXIS_MAX ) {
        return SDL_JoystickGetButton( joystick->sdlJoystick(), value );
    }

    // else it's an axis value.
    return SDL_JoystickGetAxis( joystick->sdlJoystick(), value );
}

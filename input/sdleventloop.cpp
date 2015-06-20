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

    Q_INIT_RESOURCE( controllerdb );
    QFile file( ":/input/gamecontrollerdb.txt" );
    file.open( QIODevice::ReadOnly );

    auto mappingData = file.readAll();

    if( SDL_SetHint( SDL_HINT_GAMECONTROLLERCONFIG, mappingData.constData() ) == SDL_FALSE ) {
        qFatal( "Fatal: Unable to load controller database: %s", SDL_GetError() );
    }

    file.close();

    for( int i = 0; i < Joystick::maxNumOfDevices; ++i ) {
        sdlDeviceList.append( nullptr );
    }


    sdlPollTimer.setInterval( 5 );

    connect( &sdlPollTimer, &QTimer::timeout, this, &SDLEventLoop::processEvents );

    initSDL();

}

void SDLEventLoop::processEvents() {

    if( !forceEventsHandling ) {

        SDL_GameControllerUpdate();
        SDL_JoystickUpdate();

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

            joystick->insert( InputDeviceEvent::L2, leftTrigger );
            joystick->insert( InputDeviceEvent::R2, rightTrigger );

            Q_UNUSED( rightXAxis );
            Q_UNUSED( rightYAxis );

            if( !joystick->analogMode() ) {

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
            }

            joystick->insert( InputDeviceEvent::Up, up );
            joystick->insert( InputDeviceEvent::Down, down );
            joystick->insert( InputDeviceEvent::Left, left );
            joystick->insert( InputDeviceEvent::Right, right );

        }
    }

    else {
        SDL_Event sdlEvent;

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


                    qCDebug( phxInput ) << "Controller Added: " << joystick->name();
                    break;
                }

                case SDL_CONTROLLERDEVICEREMOVED: {

                    int index = deviceLocationMap.value( sdlEvent.cbutton.which, -1 );

                    Q_ASSERT( index != -1 );

                    auto *joystick = sdlDeviceList.at( index );

                    Q_ASSERT( joystick != nullptr );

                    if( joystick->instanceID() == sdlEvent.cdevice.which ) {
                        qCDebug( phxInput ) << "Controller Removed: " << joystick->name();
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

void SDLEventLoop::initSDL() {

    if( SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 ) {
        qFatal( "Fatal: Unable to initialize SDL2: %s", SDL_GetError() );
    }

    SDL_GameControllerEventState( SDL_ENABLE );
}

void SDLEventLoop::quitSDL() {
    SDL_Quit();
}

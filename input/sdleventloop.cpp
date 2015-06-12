#include "sdleventloop.h"

#include "logging.h"

#include <QFile>
#include <QMutexLocker>


SDLEventLoop::SDLEventLoop( QObject *parent )
    : QObject( parent ),
      sdlPollTimer( this ),
      numOfDevices( 0 ),
      forceEventsHandling( true )
{
    // To do the poll timer isn't in the sdlEventLoopThread. it needs to be.

    Q_INIT_RESOURCE( controllerdb );
    QFile file( ":/input/gamecontrollerdb.txt" );
    Q_ASSERT( file.open( QIODevice::ReadOnly ) );

    auto mappingData = file.readAll();
    if ( SDL_SetHint( SDL_HINT_GAMECONTROLLERCONFIG, mappingData.constData() ) == SDL_FALSE )
        qFatal( "Fatal: Unable to load controller database: %s", SDL_GetError() );


    for ( int i=0; i < Joystick::maxNumOfDevices; ++i )
        sdlDeviceList.append( nullptr );


    sdlPollTimer.setInterval( 5 );

    connect( &sdlPollTimer, &QTimer::timeout, this, &SDLEventLoop::processEvents );

    initSDL();

}

void SDLEventLoop::processEvents() {

    if ( !forceEventsHandling ) {
        SDL_GameControllerUpdate();
        for ( auto &key : deviceLocationMap.keys() ) {
            auto index = deviceLocationMap[ key ];

            auto *joystick = sdlDeviceList.at( index );
            auto *sdlGamepad = joystick->sdlDevice();

            // Check to see if sdlGamepad is actually connected. If it isn't this will terminate the
            // polling and initialize the event handling.

            forceEventsHandling = SDL_GameControllerGetAttached( sdlGamepad ) == SDL_FALSE;
            if ( forceEventsHandling )
                return;

            // Get all button states
            bool up = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_DPAD_UP );
            bool down = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN  );
            bool left = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT  );
            bool right = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT  );
            bool start = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_START  );
            bool back = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_BACK  );
            bool b = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_B  );
            bool a = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_A  );
            bool x = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_X  );
            bool y = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_Y  );
            bool leftShoulder = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER  );
            bool rightShoulder = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER  );
            bool leftStick = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK );
            bool rightStick = SDL_GameControllerGetButton( sdlGamepad, SDL_CONTROLLER_BUTTON_RIGHTSTICK );

            // Insert all button states except for directional buttons
            joystick->insert( InputDeviceEvent::Start, start );
            joystick->insert( InputDeviceEvent::Select, back );
            joystick->insert( InputDeviceEvent::B, b );
            joystick->insert( InputDeviceEvent::A, a );
            joystick->insert( InputDeviceEvent::X, x );
            joystick->insert( InputDeviceEvent::Y, y );
            joystick->insert( InputDeviceEvent::L, leftShoulder );
            joystick->insert( InputDeviceEvent::R, rightShoulder );
            joystick->insert( InputDeviceEvent::L3, leftStick );
            joystick->insert( InputDeviceEvent::R3, rightStick );

            int16_t leftXAxis = SDL_GameControllerGetAxis( sdlGamepad, SDL_CONTROLLER_AXIS_LEFTX  );
            int16_t leftYAxis = SDL_GameControllerGetAxis( sdlGamepad, SDL_CONTROLLER_AXIS_LEFTY  );

            int16_t rightXAxis = SDL_GameControllerGetAxis( sdlGamepad, SDL_CONTROLLER_AXIS_RIGHTX  );
            int16_t rightYAxis = SDL_GameControllerGetAxis( sdlGamepad, SDL_CONTROLLER_AXIS_RIGHTY  );

            Q_UNUSED( rightXAxis );
            Q_UNUSED( rightYAxis );


            if ( !joystick->analogMode() ) {

                if ( !left ) {
                    if ( leftXAxis <= 0 ) {
                        left = ( leftXAxis < -joystick->deadZone() );
                    }
                }

                if ( !down ) {
                    if ( leftXAxis > 0 ) {
                        right = ( leftXAxis > joystick->deadZone() );
                    }
                }

                if ( !up) {
                    if (  leftYAxis <= 0 ) {
                        up = ( leftYAxis < -joystick->deadZone() );
                    }
                }

                if ( !down ) {
                    if ( leftYAxis > 0 ) {
                        down = ( leftYAxis > joystick->deadZone() );
                    }
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

                    if ( sdlDeviceList.at( sdlEvent.cdevice.which ) != nullptr ) {
                        qCDebug( phxInput ) << "Device already exists at " << sdlEvent.cdevice.which;
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

                    if ( joystick->instanceID() == sdlEvent.cdevice.which ) {
                        qCDebug( phxInput ) << "Controller Removed: " << joystick->name();
                        emit deviceRemoved( joystick->sdlIndex() );
                        sdlDeviceList[ index ] = nullptr;
                        deviceLocationMap.remove( sdlEvent.cbutton.which );
                        forceEventsHandling = true;
                        break;
                    }

                    break;
                }

                    /*
                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
                    buttonChanged( sdlEvent.cbutton );
                    break;
                }

                case SDL_JOYAXISMOTION:
                case SDL_CONTROLLERAXISMOTION: {
                    int index = deviceLocationMap.value( sdlEvent.cbutton.which );
                    auto *device = sdlDeviceList.at( index );

                    if ( !device )
                        break;

                    bool pressed = ( qAbs( sdlEvent.caxis.value ) > device->deadZone() );

                    Q_UNUSED( pressed );
                    // The lowest value is 0, so 0 + 4 will put us at the proper retropad value, ranging
                    // from 4 to 7;
                    //sdlDeviceList.at( index )->insert( sdlEvent.caxis.axis + 4, pressed );
                    //qDebug() << "Axis motion: " << pressed << sdlEvent.caxis.axis + 4;
                    //qDebug() << sdlEvent.caxis.type;
                    //OnControllerAxis( sdlEvent.caxis );
                    break;
                }

                case SDL_JOYHATMOTION: {
                    int index = deviceLocationMap.value( sdlEvent.cbutton.which );
                    auto *device = sdlDeviceList.at( index );
                    Q_UNUSED( device )
                    break;
                }
                */
                // YOUR OTHER EVENT HANDLING HERE
                default:
                    //qDebug() << "hit";
                    //qDebug() << "Default: " << sdlEvent.text.text;
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

void SDLEventLoop::buttonChanged( SDL_ControllerButtonEvent &controllerButton ) {

        auto inputEvent = InputDeviceEvent::Unknown;

        switch ( controllerButton.button ) {
            case SDL_CONTROLLER_BUTTON_A:
                inputEvent = InputDeviceEvent::A;
                break;
            case SDL_CONTROLLER_BUTTON_B:
                inputEvent = InputDeviceEvent::B ;
                break;
            case SDL_CONTROLLER_BUTTON_X:
                inputEvent = InputDeviceEvent::X ;
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                inputEvent = InputDeviceEvent::Y ;
                break;
            case SDL_CONTROLLER_BUTTON_BACK:
                inputEvent = InputDeviceEvent::Select ;
                break;
            case SDL_CONTROLLER_BUTTON_GUIDE:
                break;
            case SDL_CONTROLLER_BUTTON_START:
                inputEvent = InputDeviceEvent::Start ;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                inputEvent = InputDeviceEvent::Up ;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                inputEvent = InputDeviceEvent::Down ;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                inputEvent = InputDeviceEvent::Left ;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                inputEvent = InputDeviceEvent::Right ;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                inputEvent = InputDeviceEvent::L ;
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                inputEvent = InputDeviceEvent::R ;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                inputEvent = InputDeviceEvent::L3 ;
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                inputEvent = InputDeviceEvent::R3 ;
                break;
        }


        if ( inputEvent != InputDeviceEvent::Unknown ) {

            int index = deviceLocationMap.value( controllerButton.which );
            auto *device = sdlDeviceList.at( index );

            // SDL checks all the states of the buttons when a device is delete,
            // so we need to make sure the device is actually valid.
            if ( !device )
                return;

            device->insert( new InputDeviceEvent( inputEvent
                                                 , controllerButton.state == SDL_PRESSED
                                                 , device ) );
        }



}



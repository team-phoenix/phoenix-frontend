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

            int16_t left = 0;
            int16_t right = 0;
            int16_t down = 0;
            int16_t up = 0;

            for ( auto &sdlEvent : joystick->mapping().keys() ) {
                auto deviceEvent = joystick->mapping().value( sdlEvent );

                int16_t state = 0;

                if ( deviceEvent == InputDeviceEvent::Axis ) {

                    // Immitate a D-PAD.
                    if ( !joystick->analogMode() ) {

                        if ( sdlEvent == "leftx" ) {
                            state = SDL_JoystickGetAxis( joystick->sdlJoystick()
                                                         , joystick->sdlMapping().value( sdlEvent ) );
                            left |= ( state < -joystick->deadZone() );

                            right |= ( state > joystick->deadZone() );

                        }

                        else if ( sdlEvent == "lefty" ) {

                            state = SDL_JoystickGetAxis( joystick->sdlJoystick()
                                                         , joystick->sdlMapping().value( sdlEvent ) );

                            up |= ( state < -joystick->deadZone() );
                            down |= ( state > joystick->deadZone() );

                            //qDebug() << up << down;
                        }

                    }

                    else {
                        // Still need to handle true analog events.
                    }
                }

                else { // InputDeviceEvent is a button value.
                    state = SDL_JoystickGetButton( joystick->sdlJoystick()
                                                   , joystick->sdlMapping().value( sdlEvent ) );

                    if ( deviceEvent == InputDeviceEvent::Left )
                        left |= state;
                    else if ( deviceEvent == InputDeviceEvent::Right )
                        right |= state;
                    else if ( deviceEvent == InputDeviceEvent::Up )
                        up |= state;
                    else if ( deviceEvent == InputDeviceEvent::Down  )
                        down |= state;

                    else {
                        joystick->insert( deviceEvent, state );
                    }

                }
            }

            joystick->insert( InputDeviceEvent::Left, left );
            joystick->insert( InputDeviceEvent::Right, right );
            joystick->insert( InputDeviceEvent::Up, up );
            joystick->insert( InputDeviceEvent::Down, down );

           // qDebug() << joystick->value( InputDeviceEvent::L) << joystick->value( InputDeviceEvent::R);

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

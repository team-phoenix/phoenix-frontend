#include "sdleventloop.h"

#include "logging.h"


SDLEventLoop::SDLEventLoop( QObject *parent )
    : QObject( parent ),
      sdlPollTimer( this ),
      numOfDevices( 0 )
{
    // To do the poll timer isn't in the sdlEventLoopThread. it needs to be.

    this->moveToThread( &sdlEventLoopThread );

    sdlPollTimer.moveToThread( &sdlEventLoopThread );

    sdlPollTimer.setInterval( 10 );

    connect( &sdlEventLoopThread, &QThread::started, this, &SDLEventLoop::startTimer );
    connect( &sdlEventLoopThread, &QThread::finished, &sdlPollTimer, &QTimer::stop );
    connect( &sdlPollTimer, &QTimer::timeout, this, &SDLEventLoop::processEvents );

    initSDL();

    //qDebug() << sdlEventLoopThread.thread() << sdlPollTimer.thread() << this->thread();


}

void SDLEventLoop::processEvents()
{
    SDL_Event sdlEvent;

    while( SDL_PollEvent( &sdlEvent ) ) {

        switch( sdlEvent.type ) {

            case SDL_JOYDEVICEADDED: {
                mutex.lock();

                qCDebug( phxInput ) << "Current Devices: " << numOfDevices << ", New Devices: " <<  SDL_NumJoysticks();

                if ( numOfDevices != SDL_NumJoysticks() ) {
                    quitSDL();
                    initSDL();
                    findJoysticks();
                }


/*
                if( SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 ) {
                    qFatal( "Fatal: Unable to initialize SDL2: %s", SDL_GetError() );
                }*/

                //AddController( sdlEvent.cdevice );
                mutex.unlock();
                break;
            }
            case SDL_CONTROLLERDEVICEADDED: {
                qCDebug( phxInput ) << "Controller Device Added: " << sdlEvent.cdevice.which;
                break;
            }

            case SDL_JOYDEVICEREMOVED: {
                mutex.lock();
                //quitSDL();
                //initSDL();
                qCDebug( phxInput ) << "JOYSTICK REMOVED";
                //RemoveController( sdlEvent.cdevice );
                mutex.unlock();
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED:
                qCDebug( phxInput ) << "GAME CONTROLLER REMOVED";
                break;

            case SDL_JOYBUTTONUP:
            case SDL_JOYBUTTONDOWN:
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP: {
                bool pressed = sdlEvent.cbutton.state == SDL_PRESSED;
                sdlDeviceList.at( sdlEvent.cbutton.which )->insert( sdlEvent.cbutton.button, pressed );
                //qDebug() << sdlEvent.cbutton.button;
                break;
            }

            case SDL_JOYAXISMOTION:
            case SDL_CONTROLLERAXISMOTION: {
                auto *device = sdlDeviceList.at( sdlEvent.cbutton.which );
                bool pressed = ( sdlEvent.caxis.value > device->deadZone() );
                sdlDeviceList.at( sdlEvent.cbutton.which )->insert( sdlEvent.caxis.axis + 4, pressed );
                //qDebug() << "Axis motion: " << pressed << sdlEvent.caxis.axis + 4;
                //qDebug() << sdlEvent.caxis.type;
                //OnControllerAxis( sdlEvent.caxis );
                break;
            }

            case SDL_JOYHATMOTION:
                qDebug() << "hat mootion " << sdlEvent.jhat.hat << " with value " << sdlEvent.jhat.value;
                break;

                // YOUR OTHER EVENT HANDLING HERE
            default:
                //qDebug() << "hit";
                //qDebug() << "Default: " << sdlEvent.text.text;
                break;

        }
    }
}

void SDLEventLoop::initSDL()
{
    if( SDL_Init( SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 ) {
        qFatal( "Fatal: Unable to initialize SDL2: %s", SDL_GetError() );
    }

    SDL_JoystickEventState( SDL_ENABLE );
}

void SDLEventLoop::findJoysticks()
{
    numOfDevices = SDL_NumJoysticks();

    for ( int i=0; i < numOfDevices; ++i ) {
        auto *joystick = new Joystick( i );
        sdlDeviceList.insert( i, joystick );
        emit deviceConnected( joystick );
    }
}

void SDLEventLoop::quitSDL()
{
    SDL_Quit();
}



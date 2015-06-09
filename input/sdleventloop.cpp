#include "sdleventloop.h"

#include "logging.h"

#include <QFile>
#include <QResource>


SDLEventLoop::SDLEventLoop( QObject *parent )
    : QObject( parent ),
      sdlPollTimer( this ),
      numOfDevices( 0 )
{
    // To do the poll timer isn't in the sdlEventLoopThread. it needs to be.

    Q_INIT_RESOURCE( controllerdb );
    QFile file( ":/input/gamecontrollerdb.txt" );
    Q_ASSERT( file.open( QIODevice::ReadOnly ) );

    auto mappingData = file.readAll();
    if ( SDL_SetHint( SDL_HINT_GAMECONTROLLERCONFIG, mappingData.constData() ) == SDL_FALSE )
        qFatal( "Fatal: Unable to load controller database: %s", SDL_GetError() );

    this->moveToThread( &sdlEventLoopThread );

    for ( int i=0; i < Joystick::maxNumOfDevices; ++i )
        sdlDeviceList.append( nullptr );

    sdlPollTimer.moveToThread( &sdlEventLoopThread );

    sdlPollTimer.setInterval( 5 );

    connect( &sdlEventLoopThread, &QThread::started, this, &SDLEventLoop::startTimer );
    connect( &sdlEventLoopThread, &QThread::finished, &sdlPollTimer, &QTimer::stop );
    connect( &sdlPollTimer, &QTimer::timeout, this, &SDLEventLoop::processEvents );

    initSDL();

    qDebug() << sdlEventLoopThread.thread() << sdlPollTimer.thread() << this->thread();


}

void SDLEventLoop::processEvents() {
    SDL_Event sdlEvent;

    while( SDL_PollEvent( &sdlEvent ) ) {

        switch( sdlEvent.type ) {

            case SDL_CONTROLLERDEVICEADDED: {

                if ( sdlDeviceList.at( sdlEvent.cdevice.which ) != nullptr ) {
                    qCDebug( phxInput ) << "Device already exists at " << sdlEvent.cdevice.which;
                    break;
                }

                auto *joystick = new Joystick( sdlEvent.cdevice.which );
                sdlDeviceList.insert( sdlEvent.cdevice.which, joystick );
                emit deviceConnected( joystick );

                qCDebug( phxInput ) << "Controller Device Added: " << sdlEvent.cdevice.which;
                break;
            }


            case SDL_CONTROLLERDEVICEREMOVED: {
                //delete sdlDeviceList.at( sdlEvent.cdevice.which );
                //emit deviceRemoved( Joystick(sdlEvent.cdevice.which );

                qCDebug( phxInput ) << "GAME CONTROLLER REMOVED";
                break;
            }

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP: {
                buttonChanged( sdlEvent.cbutton );
                break;
            }

            case SDL_JOYAXISMOTION:
            case SDL_CONTROLLERAXISMOTION: {
                auto *device = sdlDeviceList.at( sdlEvent.cbutton.which );
                bool pressed = ( qAbs( sdlEvent.caxis.value ) > device->deadZone() );

                // The lowest value is 0, so 0 + 4 will put us at the proper retropad value, ranging
                // from 4 to 7;
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
            auto *device = sdlDeviceList.at( controllerButton.which );
            device->insert( new InputDeviceEvent( inputEvent
                                                 , controllerButton.state == SDL_PRESSED
                                                 , device ) );
        }



}



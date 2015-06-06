#include "inputmanager.h"

InputManager::InputManager(QObject *parent)
    : QObject(parent),
      keyboard( new Keyboard() ),
      qmlCurrentIndex( 0 ),
      qmlCurrentItem( nullptr )
{


    connect( &sdlEventLoop, &SDLEventLoop::deviceConnected, this, &InputManager::append );

    // The Keyboard will be always active in port 0,
    // unless changed by the user.
    keyboardActivated = true;


    sdlEventLoop.start();
}

InputManager::~InputManager()
{
    sdlEventLoop.stop();

    for ( auto device : deviceList ) {
        if ( device )
            device->deleteLater();
    }

    //sdlThread.requestInterruption();
    //sdlThread.wait();



}


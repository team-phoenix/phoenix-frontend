#include "inputmanager.h"

InputManager::InputManager( QObject *parent )
    : QObject( parent ),
      sdlEventLoop( this ),
      keyboard( new Keyboard() )
{


    connect( &sdlEventLoop, &SDLEventLoop::deviceConnected, this, &InputManager::insert );
    connect( &sdlEventLoop, &SDLEventLoop::deviceRemoved, this, &InputManager::removeAt );

    // The Keyboard will be always active in port 0,
    // unless changed by the user.
    keyboardActivated = true;

    for ( int i=0; i < Joystick::maxNumOfDevices; ++i )
        deviceList.append( nullptr );

    sdlEventLoop.start();

    qDebug() << "Input Manager thread: " << this->thread();

}

InputManager::~InputManager() {
    sdlEventLoop.stop();

    // I can't guarantee that the device won't be deleted by the deviceRemoved() signal.
    // So make sure we check.

    for ( auto device : deviceList ) {
        if( device ) {
            device->deleteLater();
        }
    }


    //sdlThread.requestInterruption();
    //sdlThread.wait();

}

void InputManager::insert(InputDevice *device)
{
    mutex.lock();
    auto *joystick = static_cast<Joystick *>( device );

    if ( deviceList.first() == nullptr || deviceList.first() == keyboard ) {
        joystick->setSDLIndex( 0 );
        keyboard->shareStates( joystick );
    }

    deviceList[ joystick->sdlIndex() ] = joystick;

    mutex.unlock();
    emit deviceAdded( joystick );

}

void InputManager::removeAt( int index )
{
    mutex.lock();

    auto *device = static_cast<Joystick *>( deviceList.at( index ) );

    delete device;

    deviceList[ index ] = nullptr;

    if ( deviceList.first() == nullptr ) {
        deviceList[ 0 ] = keyboard;
    }

    mutex.unlock();

}

bool InputManager::shareDeviceStates(const int index)
{
    if( index < deviceList.length() )
    {
        return keyboard->shareStates( deviceList.at( index ) );
    }

    return false;
}

bool InputManager::shareDeviceStates(const int index1, const int index2)
{
    if( index1 < deviceList.length() && index2 < deviceList.length() ) {
        deviceList.at( index1 )->shareStates( deviceList.at( index2 ) );
    }

    return false;
}


#include "inputmanager.h"

InputManager::InputManager( QObject *parent )
    : QObject( parent ),
      sdlEventLoop( this ),
      keyboard( new Keyboard() ) {


    connect( &sdlEventLoop, &SDLEventLoop::deviceConnected, this, &InputManager::insert );
    connect( &sdlEventLoop, &SDLEventLoop::deviceRemoved, this, &InputManager::removeAt );

    // The Keyboard will be always active in port 0,
    // unless changed by the user.
    keyboardActivated = true;

    for( int i = 0; i < Joystick::maxNumOfDevices; ++i ) {
        deviceList.append( nullptr );
    }

    sdlEventLoop.start();

}

InputManager::~InputManager() {
    sdlEventLoop.stop();

    // I can't guarantee that the device won't be deleted by the deviceRemoved() signal.
    // So make sure we check.

    for( auto device : deviceList ) {
        if( device ) {
            device->deleteLater();
        }
    }


}

bool InputManager::isKeyboardActive() const {
    return keyboardActivated;
}

int InputManager::size() const {
    return deviceList.size();
}

InputDevice *InputManager::at(int index) {

    QMutexLocker locker( &mutex );
    return deviceList.at( index );
}

void InputManager::pollStates()
{
    sdlEventLoop.pollEvents();

}

void InputManager::insert( InputDevice *device ) {
    mutex.lock();
    auto *joystick = static_cast<Joystick *>( device );

    deviceList[ joystick->sdlIndex() ] = joystick;

    mutex.unlock();
    emit deviceAdded( joystick );

}

void InputManager::removeAt( int index ) {
    mutex.lock();

    auto *device = static_cast<Joystick *>( deviceList.at( index ) );

    delete device;

    deviceList[ index ] = nullptr;

    if( deviceList.first() == nullptr ) {
        deviceList[ 0 ] = keyboard;
    }

   mutex.unlock();

}

void InputManager::setRun(bool run) {
    mutex.lock();

    if( run ) {
        sdlEventLoop.stop();
        for( auto device : deviceList ) {
            if( device ) {
                device->setEditMode( false );
            }
        }
    }

    else {
        sdlEventLoop.start();
    }

    mutex.unlock();

}

void InputManager::swap(const int index1, const int index2) {
    deviceList.swap( index1, index2 );
}

void InputManager::emitConnectedDevices() {
    emit deviceAdded( keyboard );

    for( auto inputDevice : deviceList ) {
        if( inputDevice ) {
            emit deviceAdded( inputDevice );
        }
    }
}


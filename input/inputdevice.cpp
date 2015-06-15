#include "inputdevice.h"

#include <QSharedPointer>

InputDevice::InputDevice( const InputDevice::LibretroType type, const QString name, QObject *parent )
    : QObject( parent ),
      deviceStates( new InputStateMap {
    { InputDeviceEvent::B, false },
    { InputDeviceEvent::A, false },
    { InputDeviceEvent::X, false },
    { InputDeviceEvent::Y, false },
    { InputDeviceEvent::Up, false },
    { InputDeviceEvent::Down, false },
    { InputDeviceEvent::Right, false },
    { InputDeviceEvent::Left, false },
    { InputDeviceEvent::R, false },
    { InputDeviceEvent::L, false },
    { InputDeviceEvent::L2, false },
    { InputDeviceEvent::R2, false },
    { InputDeviceEvent::R3, false },
    { InputDeviceEvent::L3, false },
    { InputDeviceEvent::Start, false },
    { InputDeviceEvent::Select, false },
} ),
deviceType( type ),
sharingStates( true ),
deviceName( name ),
qmlEditMode( false ) {

    setRetroButtonCount( 15 );
}

InputDevice::~InputDevice() {

}

InputDevice::InputDevice( const InputDevice::LibretroType type, QObject *parent )
    : InputDevice( type, "No-Name", parent )

{

}

const QString InputDevice::name() const {
    return deviceName;
}

QString InputDevice::mappingString() const {
    return qmlMappingString;
}

int16_t InputDevice::value( const InputDeviceEvent::Event &event, const int16_t defaultValue ) {
    mutex.lock();
    auto pressed = deviceStates->value( event, defaultValue );
    mutex.unlock();
    return pressed;
}

bool InputDevice::contains( const InputDeviceEvent::Event &event ) {
    return value( event, defaultValue ) != defaultValue;
}

void InputDevice::insert( const InputDeviceEvent::Event &value, const int16_t &state ) {
    mutex.lock();
    deviceStates->insert( value, state );
    mutex.unlock();
}

void InputDevice::insert( InputDeviceEvent *event ) {
    if( editMode() ) {
        emit inputDeviceEventChanged( event );
    } else {

        insert( static_cast<InputDeviceEvent::Event>( event->value() ), event->state() );
    }

    delete event;
}

InputDevice::LibretroType InputDevice::type() const {
    return deviceType;
}

InputStateMap *InputDevice::states() {
    return deviceStates;
}

bool InputDevice::shareStates( InputDevice *device ) {
    Q_ASSERT_X( this != device, "InputDevice::shareStates", "cannot share state with itself." );

    if( device == nullptr ) {
        if( sharingEnabled() ) {
            resetStates();
        }

        return false;
    }

    deviceStates = device->states();
    return true;

}

void InputDevice::resetStates() {
    for( auto &key : deviceStates->keys() ) {
        deviceStates->value( key, false );
    }
}

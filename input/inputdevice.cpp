#include "inputdevice.h"

#include <QSharedPointer>

//
// Constructors
//

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

InputDevice::InputDevice( const InputDevice::LibretroType type, QObject *parent )
    : InputDevice( type, "Unknown", parent ) {

}

//
// Public
//

const QString InputDevice::name() const {
    return deviceName;
}

bool InputDevice::editMode() const {
    return qmlEditMode;
}

int InputDevice::retroButtonCount() const {
    return qmlRetroButtonCount;
}

QString InputDevice::mappingString() const {
    return qmlMappingString;
}

InputDevice::LibretroType InputDevice::type() const {
    return deviceType;
}

InputStateMap *InputDevice::states() {
    return deviceStates;
}

InputDeviceMapping &InputDevice::mapping() {
    return deviceMapping;
}

void InputDevice::setName( const QString name ) {
    deviceName = name;
    emit nameChanged();
}

void InputDevice::setEditMode( const bool edit ) {
    qmlEditMode = edit;
    emit editModeChanged();
}

void InputDevice::setType( const InputDevice::LibretroType type ) {
    deviceType = type;
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

bool InputDevice::isSharingStates() const {
    return sharingStates;
}

bool InputDevice::sharingEnabled() const {
    return sharingStates;
}

//
// Public slots
//

int16_t InputDevice::value( const InputDeviceEvent::Event &event, const int16_t defaultValue ) {
    mutex.lock();
    auto pressed = deviceStates->value( event, defaultValue );
    mutex.unlock();
    return pressed;
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

bool InputDevice::contains( const InputDeviceEvent::Event &event ) {
    return value( event, ~0) != ~0;
}

void InputDevice::setMapping( const QVariantMap mapping ) {
    Q_UNUSED( mapping );
}

//
// Private
//

void InputDevice::resetStates() {
    for( auto &key : deviceStates->keys() ) {
        deviceStates->value( key, false );
    }
}

void InputDevice::setRetroButtonCount( const int count ) {
    qmlRetroButtonCount = count;
    emit retroButtonCountChanged();
}

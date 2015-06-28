#include "inputdevice.h"

//
// Constructors
//

bool InputDevice::gamepadControlsFrontend = false;

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
    qmlResetMapping( false ),
    deviceType( type ),
    sharingStates( true ),
    deviceName( name ),
    qmlEditMode( false ) {
    setRetroButtonCount( 15 );
}

InputDevice::InputDevice( QObject *parent )
    : InputDevice( DigitalGamepad, parent )
{

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

bool InputDevice::resetMapping() const
{
    return qmlResetMapping;
}

InputDevice::LibretroType InputDevice::type() const {
    return deviceType;
}

InputStateMap *InputDevice::states() {
    return deviceStates;
}

void InputDevice::setName( const QString name ) {
    deviceName = name;
    emit nameChanged();
}

void InputDevice::setEditMode( const bool edit ) {
    qmlEditMode = edit;
    emit editModeChanged();
}

void InputDevice::setResetMapping( const bool reset )
{
    qmlResetMapping = reset;
    emit resetMappingChanged();
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

void InputDevice::saveMapping()
{
    /*
    QSettings settings;
    settings.beginGroup( name() );

    for ( auto &key : mapping().keys() ) {
        auto value = mapping().value( key );
        settings.setValue( InputDeviceEvent::toString( value ), key );
    }

    qDebug() << settings.fileName();
    */
}

bool InputDevice::loadMapping()
{
    /*
    QSettings settings;

    if ( !QFile::exists( settings.fileName() ) )
        return false;

    settings.beginGroup( name() );

    for ( int i=0; i < InputDeviceEvent::Unknown; ++i ) {
        auto event = static_cast<InputDeviceEvent::Event>( i );
        auto eventString = InputDeviceEvent::toString( event );

        auto key = settings.value( eventString );
        if ( key.isValid() ) {
            mapping().insert( key.toString(), event );
        }
    }

    return !mapping().isEmpty();
    */
}

void InputDevice::selfDestruct()
{
    saveMapping();
    delete this;
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
    if ( InputDevice::gamepadControlsFrontend ) {
        emit inputDeviceEvent( value, state );
    }
    deviceStates->insert( value, state );
    mutex.unlock();
}

void InputDevice::setMapping(const QVariantMap mapping)
{
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

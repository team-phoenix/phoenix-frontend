#include "keyboard.h"

Keyboard::Keyboard( QObject *parent )
    : InputDevice( LibretroType::DigitalGamepad, "Keyboard", parent ) {

    loadDefaultMapping();

}

void Keyboard::loadDefaultMapping()
{
    mapping().insert( QString::number( Qt::Key_A ) , InputDeviceEvent::A );
    mapping().insert( QString::number( Qt::Key_D ) , InputDeviceEvent::B );
    mapping().insert( QString::number( Qt::Key_W ) , InputDeviceEvent::Y );
    mapping().insert( QString::number( Qt::Key_S ) , InputDeviceEvent::X );
    mapping().insert( QString::number( Qt::Key_Up ) , InputDeviceEvent::Up );
    mapping().insert( QString::number( Qt::Key_Down ) , InputDeviceEvent::Down );
    mapping().insert( QString::number( Qt::Key_Right ) , InputDeviceEvent::Right );
    mapping().insert( QString::number( Qt::Key_Left ) , InputDeviceEvent::Left );
    mapping().insert( QString::number( Qt::Key_Space ) , InputDeviceEvent::Select );
    mapping().insert( QString::number( Qt::Key_Return ), InputDeviceEvent::Start );
    mapping().insert( QString::number( Qt::Key_Z ), InputDeviceEvent::L );
    mapping().insert( QString::number( Qt::Key_X ), InputDeviceEvent::R );
    mapping().insert( QString::number( Qt::Key_P ), InputDeviceEvent::L2 );
    mapping().insert( QString::number( Qt::Key_Shift ) , InputDeviceEvent::R2 );
    mapping().insert( QString::number( Qt::Key_N ), InputDeviceEvent::L3 );
    mapping().insert( QString::number( Qt::Key_M ), InputDeviceEvent::R3 );
}

void Keyboard::insert( const int &event, int16_t pressed ) {
    InputDeviceEvent::Event newEvent = mapping().value( QString::number( event ), InputDeviceEvent::Unknown );

    if( newEvent != InputDeviceEvent::Unknown ) {
        InputDevice::insert( newEvent, pressed );
    }
}

void Keyboard::setMapping(const QVariantMap mapping) {
    Q_UNUSED( mapping );
    // To do...
}

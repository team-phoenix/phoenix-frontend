#include "keyboard.h"

Keyboard::Keyboard( QObject *parent )
    : InputDevice( LibretroType::RetroGamepad, "Keyboard", parent ) {

}

void Keyboard::insert( const Qt::Key &event, int16_t pressed ) {
    InputDeviceEvent::Event newEvent = keyConvertor.value( event , InputDeviceEvent::Unknown );

    if( newEvent != InputDeviceEvent::Unknown ) {
        InputDevice::insert( newEvent, pressed );
    }
}


QMap< Qt::Key, InputDeviceEvent::Event> Keyboard::keyConvertor  {
    { Qt::Key_A , InputDeviceEvent::A },
    { Qt::Key_D , InputDeviceEvent::B },
    { Qt::Key_W , InputDeviceEvent::Y },
    { Qt::Key_S , InputDeviceEvent::X },
    { Qt::Key_Up , InputDeviceEvent::Up },
    { Qt::Key_Down , InputDeviceEvent::Down },
    { Qt::Key_Right , InputDeviceEvent::Right },
    { Qt::Key_Left , InputDeviceEvent::Left },
    { Qt::Key_Space , InputDeviceEvent::Select },
    { Qt::Key_Return , InputDeviceEvent::Start },
    { Qt::Key_Z , InputDeviceEvent::L },
    { Qt::Key_X , InputDeviceEvent::R },
    { Qt::Key_P , InputDeviceEvent::L2 },
    { Qt::Key_Shift , InputDeviceEvent::R2 },
    { Qt::Key_N , InputDeviceEvent::L3 },
    { Qt::Key_M , InputDeviceEvent::R3 },
};

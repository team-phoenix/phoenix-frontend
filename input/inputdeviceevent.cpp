#include "inputdeviceevent.h"


QString InputDeviceEvent::toString(const InputDeviceEvent::Event &event) {
    switch( event ) {
        case B:
            return "B";

        case A:
            return "A";

        case X:
            return "X";

        case Y:
            return "Y";

        case Start:
            return "Start";

        case Select:
            return "Select";

        case Up:
            return "Up";

        case Down:
            return "Down";

        case Left:
            return "Left";

        case Right:
            return "Right";

        case L:
            return "L";

        case R:
            return "R";

        case L2:
            return "L2";

        case R2:
            return "R3";

        case L3:
            return "L3";

        case R3:
            return "R3";

        default:
            return "Unknown";
    }
}

InputDeviceEvent::Event InputDeviceEvent::toEvent(const QString button) {
    if( button == "B" ) {
        return Event::B;
    }

    if( button == "A" ) {
        return Event::A;
    }

    if( button == "X" ) {
        return Event::X;
    }

    if( button == "Y" ) {
        return Event::Y;
    }

    if( button == "Start" ) {
        return Event::Start;
    }

    if( button == "Select" ) {
        return Event::Select;
    }

    if( button == "Up" ) {
        return Event::Up;
    }

    if( button == "Down" ) {
        return Event::Down;
    }

    if( button == "Left" ) {
        return Event::Left;
    }

    if( button == "Right" ) {
        return Event::Right;
    }

    if( button == "L" ) {
        return Event::L;
    }

    if( button == "R" ) {
        return Event::R;
    }

    if( button == "R2" ) {
        return Event::R2;
    }

    if( button == "L2" ) {
        return Event::L2;
    }

    if( button == "R3" ) {
        return Event::R3;
    }

    if( button == "L3" ) {
        return Event::L3;
    }

    return Event::Unknown;
}

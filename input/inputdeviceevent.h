#ifndef INPUTDEVICEEVENT
#define INPUTDEVICEEVENT

#include <QObject>
#include <QString>
#include <QDebug>

#include "libretro.h"

class InputDevice;


class InputDeviceEvent : public QObject {
        Q_OBJECT

        Q_PROPERTY( int state READ state )
        Q_PROPERTY( int value READ value )
        Q_PROPERTY( QString displayName READ displayName )
        Q_PROPERTY( InputDevice *attachedDevice READ attachedDevice )



    public:

        enum Event {
            B = RETRO_DEVICE_ID_JOYPAD_B,
            Y = RETRO_DEVICE_ID_JOYPAD_Y,
            Select = RETRO_DEVICE_ID_JOYPAD_SELECT,
            Start = RETRO_DEVICE_ID_JOYPAD_START,
            Up = RETRO_DEVICE_ID_JOYPAD_UP,
            Down = RETRO_DEVICE_ID_JOYPAD_DOWN,
            Left = RETRO_DEVICE_ID_JOYPAD_LEFT,
            Right = RETRO_DEVICE_ID_JOYPAD_RIGHT,
            A = RETRO_DEVICE_ID_JOYPAD_A,
            X = RETRO_DEVICE_ID_JOYPAD_X,
            L = RETRO_DEVICE_ID_JOYPAD_L,
            R = RETRO_DEVICE_ID_JOYPAD_R,
            L2 = RETRO_DEVICE_ID_JOYPAD_L2,
            R2 = RETRO_DEVICE_ID_JOYPAD_R2,
            L3 = RETRO_DEVICE_ID_JOYPAD_L3,
            R3 = RETRO_DEVICE_ID_JOYPAD_R3,
            Unknown = RETRO_DEVICE_ID_JOYPAD_R3 + 1,
        };

        Q_ENUMS( Event )


        explicit InputDeviceEvent( const int value
                                   , const int16_t state
                                   , InputDevice *device )
            : qmlState( state ),
              qmlValue( value ),
              qmlDevice( device ) {

        }

        ~InputDeviceEvent() {
        }

        int16_t state() const {
            return qmlState;
        }

        int value() const {
            return qmlValue;
        }

        QString displayName() const {
            return qmlDisplayName;
        }

        InputDevice *attachedDevice() const {
            return qmlDevice;
        }

    public:

        static QString toString( const InputDeviceEvent::Event &event ) {
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

        static Event toEvent( const QString button ) {
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




    private:


        int qmlState;
        int qmlValue;
        QString qmlDisplayName;

        // Do not delete this pointer.
        InputDevice *qmlDevice;



};

//Q_DECLARE_METATYPE( InputDeviceEvent )


#endif // INPUTDEVICEEVENT


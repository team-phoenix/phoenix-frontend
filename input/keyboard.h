#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "inputdevice.h"
#include "inputdeviceevent.h"



class Keyboard : public InputDevice {
        Q_OBJECT


    public:
        // This needs to be included for proper lookup during compliation.
        // Else the compiler will throw a warning.
        using InputDevice::insert;

        explicit Keyboard( QObject *parent = 0 );

    public slots:

        void insert( const Qt::Key &event, int16_t pressed );

    private:
        static QMap< Qt::Key, InputDeviceEvent::Event> keyConverter;



};

#endif // KEYBOARD_H

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

        bool loadMapping();

    public slots:

        void insert( const int &event, int16_t pressed );
        void setMapping( const QVariantMap mapping ) override;

    private:
        void loadDefaultMapping();

};

#endif // KEYBOARD_H

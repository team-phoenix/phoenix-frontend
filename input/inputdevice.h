#ifndef INPUTDEVICE
#define INPUTDEVICE

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QDebug>
#include <QMap>
#include <QSharedPointer>
#include <QQmlEngine>
#include <QVariantMap>

#include "libretro.h"
#include "logging.h"
#include "inputdeviceevent.h"

// InputDevice represents an abstract controller

// In normal situations, you only need to
// reimplement the insert function.

// Use alias so we don't have to type this out every time. :/
using InputStateMap = QHash<InputDeviceEvent::Event, int16_t>;
using InputDeviceMapping = QHash< int, InputDeviceEvent::Event >;

class InputDevice : public QObject {
        Q_OBJECT
        Q_PROPERTY( QString name READ name WRITE setName NOTIFY nameChanged )
        Q_PROPERTY( int retroButtonCount READ retroButtonCount NOTIFY retroButtonCountChanged )
        Q_PROPERTY( bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged )

    public:

        // Controller types from libretro's perspective
        enum  LibretroType {
            RetroGamepad = RETRO_DEVICE_JOYPAD,
            AnalogGamepad = RETRO_DEVICE_ANALOG,
        };

        // Controller types from our perspective
        enum Name {
            SDLJoypad = 0,
            QtKeyboard,
            None,
        };

        explicit InputDevice( const LibretroType type, QObject *parent = 0 );

        explicit InputDevice( const LibretroType type, const QString name, QObject *parent );

        // Getters
        const QString name() const; // QML
        bool editMode() const; // QML
        int retroButtonCount() const; // QML
        QString mappingString() const;
        LibretroType type() const;
        InputStateMap *states();
        InputDeviceMapping &mapping();

        // Setters
        void setName( const QString name ); // QML
        void setEditMode( const bool edit ); // QML
        void setType( const LibretroType type );

        // Set the input device to mirror
        bool shareStates( InputDevice *device );

        // Currently sharing states?
        bool isSharingStates() const;

        // Is sharing enabled?
        bool sharingEnabled() const;

    public slots:

        // Poll button state (getter)
        virtual int16_t value( const InputDeviceEvent::Event &event, const int16_t defaultValue = ~0 );

        // Set button state (setter)
        virtual void insert( const InputDeviceEvent::Event &value, const int16_t &state );

        // Alternate mode? What's it for?
        void insert( InputDeviceEvent *event );

        // Not sure?
        virtual bool contains( const InputDeviceEvent::Event &event );

        // Set the device -> SDL2 gamepad mapping
        virtual void setMapping( const QVariantMap mapping );

    protected:

        // The device's current state (whether certain buttons are pressed)
        InputStateMap *deviceStates;

        // Map between SDL2 -> retropad
        InputDeviceMapping deviceMapping;

    signals:

        void editModeChanged(); // QML
        void nameChanged(); // QML
        void retroButtonCountChanged(); // QML
        void inputDeviceEventChanged( InputDeviceEvent *event );

    private:

        // Type of controller this input device is
        LibretroType deviceType;

        // This is for having 2 input devices share the same port;
        // indirectly of course.
        bool sharingStates;

        // Controller states are read by a different thread, lock access with a mutex
        QMutex mutex;

        // How is this used?
        const int16_t defaultValue = ~0;

        // Clear button states
        void resetStates();
        void setRetroButtonCount( const int count );

        // QML Variables
        QString deviceName;
        QString qmlMappingString;
        int qmlRetroButtonCount;
        bool qmlEditMode;

};

Q_DECLARE_METATYPE( InputDevice * )

#endif // INPUTDEVICE


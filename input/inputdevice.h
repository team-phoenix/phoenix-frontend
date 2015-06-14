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

        enum  LibretroType {
            RetroGamepad = RETRO_DEVICE_JOYPAD,
            AnalogGamepad = RETRO_DEVICE_ANALOG,
        };

        enum Name {
            SDLJoypad = 0,
            QtKeyboard,
            None,
        };

        explicit InputDevice( const LibretroType type, QObject *parent = 0 );

        explicit InputDevice( const LibretroType type, const QString name, QObject *parent );

        ~InputDevice();

        const QString name() const;
        QString mappingString() const;

        bool editMode() const {
            return qmlEditMode;
        }

        int retroButtonCount() const {
            return qmlRetroButtonCount;
        }

        LibretroType type() const;

        InputStateMap *states();

        // This is for having 2 input devices share the same port;
        // indirectly of course.
        bool shareStates( InputDevice *device );

        void setName( const QString name ) {
            deviceName = name;
            emit nameChanged();
        }

        void setEditMode( const bool edit ) {
            qmlEditMode = edit;
            emit editModeChanged();
        }

        void setType( const LibretroType type ) {
            deviceType = type;
        }

        bool isSharingStates() const {
            return sharingStates;
        }

        bool sharingEnabled() const {
            return sharingStates;
        }

        InputDeviceMapping &mapping() {
            return deviceMapping;
        }

    public slots:

        virtual int16_t value( const InputDeviceEvent::Event &event, const int16_t defaultValue = ~0 );

        virtual bool contains( const InputDeviceEvent::Event &event );

        virtual void insert( const InputDeviceEvent::Event &value, const int16_t &state );

        void insert( InputDeviceEvent *event );

        virtual void setMapping( const QVariantMap mapping )
        {
            Q_UNUSED( mapping );
        }

    protected:
        InputStateMap *deviceStates;
        InputDeviceMapping deviceMapping;

    signals:
        void editModeChanged();
        void nameChanged();
        void retroButtonCountChanged();
        void inputDeviceEventChanged( InputDeviceEvent *event );

    private:
        LibretroType deviceType;
        bool sharingStates;
        QMutex mutex;
        const int16_t defaultValue = ~0;



        void resetStates();
        void setRetroButtonCount( const int count ) {
            qmlRetroButtonCount = count;
            emit retroButtonCountChanged();
        }

        // QML Variables
        QString deviceName;
        QString qmlMappingString;
        int qmlRetroButtonCount;
        bool qmlEditMode;
};

Q_DECLARE_METATYPE( InputDevice * )

#endif // INPUTDEVICE


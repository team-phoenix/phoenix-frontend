#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <QMap>

#include "input/inputdevice.h"
#include "libretro.h"
#include "SDL.h"

class Joystick : public InputDevice {

    public:
        // This is needed to stop a complication warning in the reimplemented insert function.
        using InputDevice::insert;

        static const int maxNumOfDevices;

        explicit Joystick( const int joystickIndex, QObject *parent = 0 );
        ~Joystick();

        // Getters
        QString guid() const;
        int buttonCount() const;
        int ballCount() const;
        int hatCount() const;
        int axisCount() const;
        int sdlIndex() const;
        qreal deadZone() const;
        bool analogMode() const;

        SDL_GameController *sdlDevice() const;
        SDL_Joystick *sdlJoystick() const;
        SDL_JoystickID instanceID() const;

        QHash< QString, int > &sdlMapping();

        void setSDLIndex( const int index );

        // This value will be set to 'true' if the
        // core detects a libretro core that
        // can use the analog sticks.

        // If not, then setting this to false
        // will cause the left analog stick
        // to mimic the D-PAD.
        void setAnalogMode( const bool mode );

        void close();
        bool loadMapping() override;
        void saveMapping() override;

    public slots:
        void insert( const quint8 &event, const int16_t pressed );
        void setMapping( QVariantMap mapping ) override;

    private:
        QString qmlGuid;
        QString qmlMappingString;
        int qmlInstanceID;
        int qmlSdlIndex;
        int qmlButtonCount;
        int qmlAxisCount;
        int qmlHatCount;
        int qmlBallCount;
        qreal qmlDeadZone;
        bool qmlAnalogMode;

        SDL_GameController *device;
        QHash<QString, int> sdlControllerMapping;

        void loadSDLMapping( SDL_GameController *device );

        // Helper function to convert a SDL styled string, into an event.
        InputDeviceEvent::Event sdlStringToEvent( const QString &key );

};

#endif // JOYSTICK_H

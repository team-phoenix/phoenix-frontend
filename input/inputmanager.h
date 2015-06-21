#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QList>
#include <QEvent>
#include <QKeyEvent>

#include "input/sdleventloop.h"
#include "input/inputdevice.h"
#include "input/keyboard.h"
#include "logging.h"

#include <memory>

class InputManager : public QObject {
        Q_OBJECT

        QMutex mutex;
        QList<InputDevice *> deviceList;

        // One keyboard is reserved for being always active.

        bool keyboardActivated;

        SDLEventLoop sdlEventLoop;

    public:
        explicit InputManager( QObject *parent = 0 );
        ~InputManager();

        Keyboard *keyboard;

        bool isKeyboardActive() const;

        int size() const;

        InputDevice *at( int index );


        void pollStates();

    public slots:

        void insert( InputDevice *device );

        void removeAt( int index );


        void setRun( bool run );

        void swap( const int index1, const int index2 );

    public slots:
        void emitConnectedDevices();

    signals:
        void device( InputDevice *device );
        void deviceAdded( InputDevice *device );
        void incomingEvent( InputDeviceEvent *event );
        void keyboardActiveChanged();

    private:

        /*
        bool eventFilter( QObject *object, QEvent *event ) {
            switch( event->type() ) {
                case QEvent::KeyPress:
                case QEvent::KeyRelease: {
                    auto *keyEvent = static_cast<QKeyEvent *>( event );
                    keyboard->insert( ( Qt::Key )keyEvent->key() , event->type() == QEvent::KeyPress );
                    event->accept();
                    break;
                }

                default:
                    return object->event( event );
            }

            return true;
        }
        */


};


#endif // INPUTMANAGER_H

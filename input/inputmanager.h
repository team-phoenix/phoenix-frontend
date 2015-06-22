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

    public:
        explicit InputManager( QObject *parent = 0 );
        ~InputManager();

        Keyboard *keyboard;

        int size() const;

        InputDevice *at( int index );

        void pollStates();

    public slots:

        // Insert or append an inputDevice to the deviceList.
        void insert( InputDevice *device );

        // Remove and delete inputDevice at index.
        void removeAt( int index );

        // Handle when the game has started playing.
        void setRun( bool run );

        // Allows the user to change controller ports.
        void swap( const int index1, const int index2 );

    public slots:
        // Iterate through, and expose inputDevices to QML.
        void emitConnectedDevices();

    signals:
        void device( InputDevice *device );
        void deviceAdded( InputDevice *device );
        void incomingEvent( InputDeviceEvent *event );

    private:
        QMutex mutex;

        QList<InputDevice *> deviceList;

        // One keyboard is reserved for being always active.


        SDLEventLoop sdlEventLoop;

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

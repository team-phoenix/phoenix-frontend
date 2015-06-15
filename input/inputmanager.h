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



        bool isKeyboardActive() const {
            return keyboardActivated;
        }

        int size() const {
            return deviceList.size();
        }

        InputDevice *at( int index ) {

            QMutexLocker locker( &mutex );
            return deviceList.at( index );
        }

    public slots:

        void insert( InputDevice *device );

        void removeAt( int index );

        bool shareDeviceStates( const int index );

        bool shareDeviceStates( const int index1, const int index2 );

        void setRun( bool run ) {
            mutex.lock();

            if( run ) {
                //sdlEventLoop.start();
                for( auto device : deviceList ) {
                    if( device ) {
                        device->setEditMode( false );
                    }
                }

                if( deviceList.first() == nullptr ) {
                    deviceList[ 0 ] = keyboard;
                }

                else {
                    keyboard->shareStates( deviceList.at( 0 ) );
                }
            }

            else {
                keyboard->shareStates( nullptr );
            }

            mutex.unlock();
            /*else {
                sdlThread.terminate();
                sdlThread.wait();
            }*/
        }

        void swap( const int index1, const int index2 ) {
            deviceList.swap( index1, index2 );
        }

    public slots:
        void emitConnectedDevices() {
            emit deviceAdded( keyboard );

            for( auto inputDevice : deviceList ) {
                if( inputDevice ) {
                    emit deviceAdded( inputDevice );
                }
            }
        }

    signals:
        void device( InputDevice *device );
        void deviceAdded( InputDevice *device );
        void incomingEvent( InputDeviceEvent *event );
        void keyboardActiveChanged();

    private:



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


};


#endif // INPUTMANAGER_H

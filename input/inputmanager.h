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

class InputManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY( InputDevice * currentItem READ currentItem NOTIFY currentItemChanged)
    Q_PROPERTY( int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged )



    QList<InputDevice *> deviceList;

    // One keyboard is reserved for being always active.

    bool keyboardActivated;

    SDLEventLoop sdlEventLoop;

    InputDevice *qmlCurrentItem;
    int qmlCurrentIndex;

public:
    explicit InputManager( QObject *parent = 0 );
    ~InputManager();



    Keyboard *keyboard;

    InputDevice *currentItem() const
    {
        return qmlCurrentItem;
    }

    int currentIndex() const
    {
        return qmlCurrentIndex;
    }

    void setCurrentIndex( const int index )
    {
        qmlCurrentIndex = index;
        emit currentIndexChanged();
        setCurrentItem( this->at( index ) );
    }


    bool isKeyboardActive() const
    {
        return keyboardActivated;
    }

    int size() const
    {
        return deviceList.size();
    }

    InputDevice *at( int index )
    {
        return deviceList.at( index );
    }

public slots:

    void append( InputDevice *device )
    {
        qCDebug( phxInput ) << "Appending device: " << device;
        if ( deviceList.isEmpty() ) {
            insert( deviceList.length(), device );
            if ( keyboard->sharingEnabled() ) {
                keyboard->shareStates( device );
            }
        }

        else {
            auto *firstDevice = deviceList.first();

            if ( device == keyboard ) {
                firstDevice = device;
                if ( keyboard->sharingEnabled() )
                    keyboard->shareStates( device );
            }

            else
                insert( deviceList.length(), device );

        }


    }

    void insert( int index, InputDevice *device )
    {
        deviceList.insert( index, device );
        if ( index == 0 )
            setCurrentIndex( index );
        emit deviceAdded( device );
    }

    bool shareDeviceStates( const int index )
    {
        if ( index < deviceList.length() )
            return keyboard->shareStates( deviceList.at( index ) );
        return false;
    }

    bool shareDeviceStates( const int index1, const int index2 )
    {
        if ( index1 < deviceList.length() && index2 < deviceList.length() ) {
            deviceList.at( index1 )->shareStates( deviceList.at( index2 ) );
        }

        return false;
    }

    void setRun( bool run )
    {

        if ( run ) {
            //sdlEventLoop.start();
            for ( auto device : deviceList ) {
                device->setEditMode( false );
            }
            if ( deviceList.isEmpty() ) {
                deviceList.insert(0, keyboard );
            }

            else {
                keyboard->shareStates( deviceList.at(0) );
            }
        }

        /*else {
            sdlThread.terminate();
            sdlThread.wait();
        }*/
    }

    void swap( const int index1, const int index2 )
    {
        deviceList.swap( index1, index2 );
    }

public slots:
    void emitConnectedDevices()
    {
        //emit device( keyboard );
        for ( auto inputDevice : deviceList )
            emit device( inputDevice );
    }

signals:
    void device( InputDevice *device );
    void deviceAdded( InputDevice *device );
    void incomingEvent( InputDeviceEvent *event );
    void keyboardActiveChanged();
    void currentIndexChanged();
    void currentItemChanged();

private:

    void setCurrentItem( InputDevice *device )
    {
        qmlCurrentItem = device;
        emit currentItemChanged();
    }

    bool eventFilter( QObject *object, QEvent *event )
    {
        switch ( event->type() ) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease: {
                auto *keyEvent = static_cast<QKeyEvent *>( event );
                keyboard->insert( (Qt::Key)keyEvent->key() , event->type() == QEvent::KeyPress );
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

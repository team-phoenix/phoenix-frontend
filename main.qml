import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1

import phoenix.video 1.0
import phoenix.input 1.0
import paths 1.0


ApplicationWindow {
    id: phoenixWindow
    title: qsTr("Phoenix");
    width: 1000 * ratio
    height: 1000
    visible: true

    property real ratio: 4/3

    function qstr(str)
    {
        return qsTr(str);
    }

    QMLInputDevice {
        id: qmlInputDevice;


        onGuideChanged: {
            if ( guide )
            console.log("guide " + guide );
        }

        onAChanged: console.log("a: " + a)
    }

    SystemPalette {
        id: systemPalette;
    }

    menuBar: MenuBar {
        Menu {
            title: "Game";
            enabled: phoenixWindow.visibility === Window.Windowed | Window.Maximized
            MouseArea {
                anchors.fill: parent;
            }


            MenuItem {
                text: "Load Game";
                onTriggered: {
                    fileDialog.type = "game";
                    fileDialog.open();
                }
            }
            MenuItem { text: "Close"; onTriggered: Qt.quit();}
        }

        Menu {
            title: "Cores";
            enabled: phoenixWindow.visibility === Window.Windowed | Window.Maximized

            Menu {
                id: coresAvailable;
                title: qstr("Available");

                Instantiator {
                    model: pathWatcherModel;
                    MenuItem {
                        text: name;
                        onTriggered: {
                            videoItem.libretroCore = path;
                        }
                    }

                    onObjectAdded: coresAvailable.insertItem(index, object);
                    onObjectRemoved: coresAvailable.removeItem(object);
                }

                MenuSeparator { }

                MenuItem {
                    text: "Clear";
                    visible: pathWatcherModel.count > 0;
                    onTriggered: {
                        pathWatcherModel.clear();
                        pathWatcherModel.append( { path: "None", name: "None" } )
                        pathWatcher.clear();
                    }
                }


            }

            MenuItem {
                text: "Change Path...";

                onTriggered: {

                    coreFolderDialog.open();
                }

            }

            MenuItem {
                text: "Load Core...";
                onTriggered: {
                    fileDialog.type = "core";
                    fileDialog.open();
                }
            }


        }

    }

    statusBar: StatusBar {
        RowLayout {
            Button {
                anchors.verticalCenter: parent.verticalCenter;
                text: qstr( "Input" );
                onClicked: {
                    if ( input.currentItem === null )
                        input.currentIndexChanged.connect( inputDialog.open() );
                    else
                        inputDialog.open();

                }
            }
        }
    }

    Dialog {

        id: inputDialog;
        visible: false;
        height: 125;
        width: 250;

        standardButtons: StandardButton.Ok | StandardButton.Cancel;

        modality: Qt.NonModal;
        function handleEvent( value, state ) {
            console.log( value, state );
        }

        contentItem: Rectangle {

            id: background;
            color: systemPalette.light;
            visible: inputDialog.visible;

            ListView {
                id: inputView;
                interactive: false;
                orientation: ListView.Vertical;
                anchors.centerIn: parent;
                height: parent.height;
                width: parent.width;

                spacing: 12;

                model: ListModel {
                    ListElement { key: "a"; value: InputDeviceEvent.A }
                    ListElement { key: "b"; value: InputDeviceEvent.B }
                    ListElement { key: "x"; value: InputDeviceEvent.X }
                    ListElement { key: "y"; value: InputDeviceEvent.Y }
                    ListElement { key: "start"; value: InputDeviceEvent.Start }
                    ListElement { key: "back"; value: InputDeviceEvent.Select }
                    ListElement { key: "dpup"; value: InputDeviceEvent.Up }
                    ListElement { key: "dpleft"; value: InputDeviceEvent.Left }
                    ListElement { key: "dpright"; value: InputDeviceEvent.Right }
                    ListElement { key: "dpdown"; value: InputDeviceEvent.Down }
                    ListElement { key: "leftstick"; value: InputDeviceEvent.L3 }
                    ListElement { key: "rightstick"; value: InputDeviceEvent.R2 }
                    ListElement { key: "leftshoulder"; value: InputDeviceEvent.L }
                    ListElement { key: "rightshoulder"; value: InputDeviceEvent.R }
                }

                delegate: Item {
                    height: 25;
                    width: 125;
                    anchors.horizontalCenter: parent.horizontalCenter;

                    Row {
                        anchors.fill: parent;
                        spacing: 12;
                        Label {
                            text: key;
                        }

                        TextField {
                            placeholderText: value;
                            onActiveFocusChanged: {
                                if ( focus ) {
                                    console.log("focus: " + key)
                                    inputView.currentIndex = index;
                                }
                            }

                        }
                    }
                }
            }
        }

    }


    PathWatcher {
        id: pathWatcher;
        Component.onCompleted: pathWatcher.start();
    }

    Connections {
        target: pathWatcher;
        onFileAdded: {
            if ( pathWatcherModel.get( 0 ).path === "None" )
                pathWatcherModel.set( 0, { path: file, name: baseName } );
            else
                pathWatcherModel.append( { path: file , name: baseName } );
        }

    }

    ListModel {
        id: pathWatcherModel;
        ListElement {path: "None"; name: "None";}
    }

    FileDialog {
        id: coreFolderDialog;
        selectFolder: true;
        onAccepted: {
            pathWatcherModel.clear();
            pathWatcherModel.append( { path: "None", name: "None" } )
            pathWatcher.clear();
            pathWatcher.slotSetCorePath(fileUrl);
        }
    }

    FileDialog {
        id: fileDialog;
        property string type: "";
        selectFolder: false;
        selectMultiple: false;
        onAccepted: {
            if (type === "core")
                videoItem.libretroCore = fileUrl;
            else if (type === "game")
                videoItem.game = fileUrl;
        }
    }

    Rectangle {

        id: videoOutput
        anchors.fill: parent
        color: "black"

        FastBlur {
            height: parent.height
            width: parent.width
            source: videoItem
            radius: 64
            rotation: 180
        }

        MouseArea {

            id: mouseArea
            anchors.fill: parent
            z: 10
            hoverEnabled: true
            onClicked:  {
                //inputDialog.visible = true;
            }

            onDoubleClicked: {

                if (phoenixWindow.visibility === Window.FullScreen)
                    phoenixWindow.visibility = Window.Windowed
                else if (phoenixWindow.visibility === Window.Windowed | Window.Maximized)
                    phoenixWindow.visibility = Window.FullScreen

            }

        }

        InputManager {
            id: input;

            property int mapIndex: 0;

            gamepadControlsFrontend: false;

            onDeviceAdded: {
                //device.resetMapping = true;
                //if ( device.name === "Keyboard" )
                  //  device.setMapping( {"a": Qt.Key_C, "b": Qt.Key_Z, "x": Qt.Key_Y } );
                console.log( device.name );
                //device.editMode = true;

                // Every device needs to forward its inputDeviceEvent signal
                // to the qmlInputDevice. This allows every controller to
                // control the UI.
                device.inputDeviceEvent.connect( function( event, state )  {
                    qmlInputDevice.insert( event, state );
                });
            }

            Component.onCompleted: {
                input.emitConnectedDevices();
            }
        }

        VideoItem {

            id: videoItem
            rotation: 180
            inputManager: input;
            anchors {
                top: parent.top
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            focus: true;
            width: height * phoenixWindow.ratio;

        }

    }

}

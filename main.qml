import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import libretro.video 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0

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



    Dialog {
        id: inputDialog;
        visible: false;
        height: 450;
        width: 250;




        /*function handleEvent( event, pressed ) {
            inputView.currentItem.newVal = event;
            console.log( "IN QML::: " + event + " state: " + pressed);
        }*/

        property var inputDevice: undefined;
        //property string inputDeviceName: undefined;
        /*onInputDeviceChanged: {
            if ( inputDevice !== undefined) {
                //inputDeviceName = inputDevice.name;

                inputDevice.inputDeviceEventChanged.connect( handleEvent );
                console.log("Got device " + inputDevice.name + " " + inputDevice.retroButtonCount );
            }
        }*/

        ListView {
            id: inputView;
            orientation: ListView.Vertical;
            model: ListModel {
                ListElement { button: "A"}
                ListElement { button: "B"}
                ListElement { button: "X"}
                ListElement { button: "Y"}
                ListElement { button: "Up"}
                ListElement { button: "Down"}
                ListElement { button: "Left"}
                ListElement { button: "Right"}
                ListElement { button: "L"}
                ListElement { button: "R"}
                ListElement { button: "R2"}
                ListElement { button: "R3"}
                ListElement { button: "L2"}
                ListElement { button: "L3"}
                ListElement { button: "Start"}
                ListElement { button: "Select"}

            }


            spacing: 3;

            height: 400;
            width: 200;

            delegate: Item {
                height: 25;
                anchors {
                    left: parent.left;
                    right: parent.right;
                }

                property string newVal: "Unknown";
                Label {
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        right: inputTextField.left;
                        rightMargin: 36;
                    }

                    text: button;

                }

                TextField {
                    id: inputTextField;

                    text: parent.newVal;
                    width: 100;
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        right: parent.right;
                        rightMargin: 12;
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
            onCurrentItemChanged: {
                currentItem.editMode = true;
                console.log("Current Button to enter is: " + mapIndex);


                currentItem.inputDeviceEventChanged.connect( function( event ) {
                    if ( event.state > 0 ) {
                        //console.log( event.value, event.state, event.displayName, event.attachedDevice );
                        if ( mapIndex > 15) {
                            console.log( "Finished Mapping" );
                            currentItem.editMode = false;
                        }
                        currentItem.insertMappingValue( event.value, input.mapIndex );
                        //input.insertMappingValue( event.value, )// This is available in all editors.
                        input.mapIndex++;
                        console.log( "Next button is " + input.mapIndex );

                    }
                });
                //console.log("CurrentItem: " + currentItem.name +
                            //" " + currentItem.retroButtonCount);

            }

            onCurrentIndexChanged: {
                //if ( currentItem !== undefined )
                    //currentItem.inputDeviceEvent.disconnect();
            }

            Component.onCompleted: {
                console.log("finished");
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

import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1

import libretro.video 1.0


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

        function handleEvent( event ) {
            if ( event.state > 0 ) {
                if ( mapIndex == maxIndex ) {
                    input.currentItem.inputDeviceEventChanged.disconnect( handleEvent );
                    inputDialog.close();
                    return;
                }
                input.currentItem.insertMappingValue( event.value, inputDialog.mapIndex );
                mapIndex++;
            }
        }

        property int mapIndex: 0;
        property int maxIndex: 15;
        property var mapValues: [
            "B", "X", "Select",
            "Start", "Up", "Down", "Left", "Right",
            "A", "X", "L", "R", "L2", "R2", "L3", "R3"
        ];

        contentItem: Rectangle {

            id: background;
            //height: 250;
            //width: 600;
            color: systemPalette.light;
            visible: inputDialog.visible;

            onVisibleChanged: {
                if ( visible ) {
                    background.state = "resetMapping";
                }
            }

            states: [

                State {

                    name: "resetMapping";

                    PropertyChanges {
                        target: rowLayout;
                        visible: true;
                    }

                    PropertyChanges {
                        target: dialogLabel;
                        text: "Edit mapping for <b>port " + inputDialog.mapIndex + "</b>?";

                    }

                },

                State {

                    name: "startMapping";

                    PropertyChanges {
                        target: inputDialog;
                        mapIndex: 0;
                    }

                    PropertyChanges {
                        target: dialogLabel;
                        text: "Press the button for <b>"
                              + inputDialog.mapValues[ inputDialog.mapIndex ]
                              + "</b>";
                    }

                    PropertyChanges {
                        target: input.currentItem;
                        editMode: true;

                    }

                }

            ]

            Label {

                id: dialogLabel;
                anchors.centerIn: parent;

            }

            RowLayout {

                id: rowLayout;
                anchors {
                    left: parent.left;
                    leftMargin: 12;
                    right: parent.right;
                    rightMargin: 12;
                    bottom: parent.bottom;
                }

                height: 50;
                spacing: 12;

                Button {
                    text: background.state == "resetMapping" ? "No" : "Close";
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        horizontalCenter: background.state == "resetMapping" ? undefined : parent.horizontalCenter;
                        right: yesButton.left;
                        rightMargin: background.state == "resetMapping" ? 12 : 0;
                    }

                    onClicked: {
                        if( background.state == "startMapping") {
                            input.currentItem.inputDeviceEventChanged.disconnect( inputDialog.handleEvent );

                        }
                        inputDialog.close();
                    }
                }

                Button {
                    id: yesButton;
                    text: "Yes";
                    visible: background.state == "resetMapping";
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        right: parent.right;
                    }


                    onClicked: {
                        input.currentItem.inputDeviceEventChanged.connect( inputDialog.handleEvent );
                        background.state = "startMapping";
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

                console.log("CurrentItem: " + currentItem.name +
                            " " + currentItem.retroButtonCount);

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

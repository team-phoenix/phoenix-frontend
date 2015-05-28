import QtQuick 2.2
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.0
import QtQuick.Layouts 1.1
import Phoenix 1.0

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


    statusBar: StatusBar {
        RowLayout {
            anchors.fill: parent;
            Button {
                text: "Options";
                anchors {
                    right: parent.right;
                    rightMargin: 24;
                }

                onClicked: {
                    if ( optionsDialog.visible )
                        optionsDialog.close();
                    else
                        optionsDialog.open();
                }
            }
        }
    }

    menuBar: MenuBar {
        Menu {
            title: "Game";
            enabled: phoenixWindow.visibility === Window.Windowed | Window.Maximized


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
            enabled: phoenixWindow.visibility === Window.Windowed | Window.Maximized;


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
                text: "Options...";
                onTriggered: optionsDialog.open();
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

    ListModel {
        id: coreVariableModel;
        ListElement { key: ""; value: ""; description: ""; choices: [
            ListElement {choice: ""}
            ]}
    }

    Dialog {
        id: optionsDialog;
        standardButtons: StandardButton.Close;
        modality: Qt.NonModal;
        height: listView.height;
        width: listView.width;

        ListView {
            id: listView;
            height: 500;
            width: 400;
            x: 12;
            spacing: 3;
            model: coreVariableModel;
            delegate: RowLayout {
                anchors {
                    left: parent.left;
                    right: parent.right;
                    rightMargin: 12;
                }

                height: 25;

                Label {
                    anchors.verticalCenter: parent.verticalCenter;
                    text: description;
                }

                ComboBox {
                    anchors {
                        right: parent.right;
                    }

                    model: choices;
                    onActivated: {
                        currentIndex = index;
                    }
                    onCurrentTextChanged: {
                        core.setVariable( key, currentText );
                    }


                }

            }
        }
/*
        contentItem: Rectangle {
            height: 300;
            width: 200;


        }
        */
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

            onDoubleClicked: {
                if (phoenixWindow.visibility === Window.FullScreen)
                    phoenixWindow.visibility = Window.Windowed
                else if (phoenixWindow.visibility === Window.Windowed | Window.Maximized)
                    phoenixWindow.visibility = Window.FullScreen

            }

        }

        VideoItem {

            id: videoItem
            rotation: 180
            anchors {
                top: parent.top
                bottom: parent.bottom
                horizontalCenter: parent.horizontalCenter
            }

            width: height * phoenixWindow.ratio;



            core: Core {
                id: core;

                onSignalUpdateVariables: {
                    var k = key;
                    var v = value;
                    var d = description;
                    var item = coreVariableModel.get(0);
                    if ( item && item.key === "" ) {
                        coreVariableModel.clear();
                    }


                    var c = choices;
                    var list = [];
                    for ( var i=0; i < c.length; ++i ) {
                        list.push( { "choice": c[i] } )
                    }

                    coreVariableModel.insert( index, {key: k, value: v, description: d, choices: list  } );

                }
            }


//            Column {
//                visible: false;
//                spacing: 24;
//                anchors.centerIn: parent;
//                Rectangle {
//                    color: "lightgray";
//                    opacity: 0.6;

//                    border {
//                        color: "white"
//                        width: 2;
//                    }

//                    height: 50;
//                    width: 175;

//                    Text {
//                        anchors.centerIn: parent;
//                        text: "Play | Pause";
//                        color: "white";
//                        font {
//                            pixelSize: 18;
//                            bold: true;
//                        }
//                    }
//                }

//                Rectangle {
//                    color: "lightgray";
//                    opacity: 0.6;
//                    border {
//                        color: "white"
//                        width: 2;
//                    }

//                    height: 50;
//                    width: 175;

//                    Text {
//                        anchors.centerIn: parent;
//                        text: "Load Game";
//                        color: "white";
//                        font {
//                            pixelSize: 18;
//                            bold: true;
//                        }
//                    }
//                }

//                Rectangle {
//                    color: "lightgray";
//                    opacity: 0.6;
//                    border {
//                        color: "white"
//                        width: 2;
//                    }

//                    height: 50;
//                    width: 175;

//                    Text {
//                        anchors.centerIn: parent;
//                        text: "Mute Audio";
//                        color: "white";
//                        font {
//                            pixelSize: 18;
//                            bold: true;
//                        }
//                    }
//                }
//            }

        }

    }

}

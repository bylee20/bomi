// written by varlesh <varlesh@gmail.com>

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithFloating {
    id: app
    name: "varlesh.Numix"
    readonly property QtObject engine: B.App.engine
    Component {
        id: sliders
        SliderStyle {
            groove: Rectangle {
                height: 4; radius: 2
                anchors.verticalCenter: parent.verticalCenter
                gradient: Gradient {
                    GradientStop {position: 0.0; color: "#fff"}
                    GradientStop {position: 1.0; color: "#fff"}
                }
                Rectangle {
                    width: parent.width*control.rate; height: parent.height
                    radius: parent.radius
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#d64937" }
                        GradientStop { position: 1.0; color: "#d64937" }
                    }
                }
            }
            handle: Image {
                width: 22; height: 22
                source: control.pressed ? "handle-pressed.png" : control.hovered ? "handle-hovered.png" : "handle.png"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    controls: Item {
        width: 400; height: inner.height+24
        BorderImage {
            id: bg; source: "bg.png"; anchors.fill: parent
            border {left: 15; right: 15; top: 15; bottom: 35}
        }
        Column {
            id: inner; width: parent.width-50; anchors.centerIn: parent; spacing: 5
            RowLayout {
                id: texts; width: parent.width; height: 15; spacing: 5
                B.TimeText {
                    id: position; time: engine.time; Layout.alignment: Qt.AlignBottom;
                    textStyle {
                        verticalAlignment: Text.AlignBottom
                        monospace: true
                        color: "white"
                        font.pixelSize: 10
                    }
                }
                Text {
                    id: name
                    Layout.alignment: Qt.AlignBottom; Layout.fillWidth: true
                    text: engine.media.name; elide: Text.ElideMiddle;
                    color: "white"; font { bold: true; pixelSize: 12 }
                    horizontalAlignment: Text.AlignHCenter
                }
                B.TimeText {
                    id: duration; time: engine.end; Layout.alignment: Qt.AlignBottom;
                    textStyle: position.textStyle
                }
            }
            RowLayout {
                id: seekbarwrapper; width: parent.width; height: 10; spacing: 10
                B.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }
            }
            Item {
                id: buttons; width: parent.width; height: 22
                RowLayout {
                    height: parent.height*0.75; anchors.verticalCenter: parent.verticalCenter; spacing: 3
                    B.Button {
                        id: mute; width: parent.height; height: parent.height
                        checked: engine.muted
                        action: "audio/volume/mute"; icon.prefix: "volume"
                    }
                    B.VolumeSlider { id: volumeslider; width: 60; style: sliders; height: parent.height }
                }
                Row {
                    height: parent.height; spacing: 20; anchors.horizontalCenter: parent.horizontalCenter;
                    B.Button {
                        id: playPrev; width: 24; height: 24
                        action: "play/prev"; icon.prefix: "previous"
                    }
                    B.Button {
                        id: playPause; width: 24; height: 24
                        action: "play/play-pause"; icon.prefix: engine.playing ? "pause" : "play"
                    }
                    B.Button {
                        id: playNext; width: 24; height: 24
                        action: "play/next"; icon.prefix: "next"
                    }
                }
            }
        }
        B.Button {
            id: toggler; parent: checked ? seekbarwrapper : buttons; icon.prefix: "toggle"
            width: 16; height: 16; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            onClicked: checked = !checked
            states: State {
                name: "toggled"; when: toggler.checked
                PropertyChanges { target:texts; opacity: 0.0; height: 0.0 }
                PropertyChanges { target:buttons; opacity: 0.0; height: 0.0 }
                PropertyChanges { target:inner; spacing: 0 }
                PropertyChanges { target:bg; border.bottom: 45 } // to adjust bg center line
            }
            transitions: Transition {
                reversible: true; to: "toggled"
                SequentialAnimation {
                    NumberAnimation { property: "opacity"; duration: 150 }
                    NumberAnimation { property: "spacing"; duration: 50 }
                    ParallelAnimation {
                        NumberAnimation { property: "height"; duration: 150 }
                        NumberAnimation { property: "border.bottom"; duration: 150 }
                    }
                }
            }
        }
        B.Button {
            id: fullscreen; width: 16; height: 16; parent: checked ? seekbarwrapper : buttons; anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 30 }
            action: "window/full"; icon.prefix: "fullscreen"
        }
        B.Button {
            id: playlistIcon; width: 16; height: 16; parent: checked ? seekbarwrapper : buttons; anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 60 }
            action: "tool/playlist/toggle"; icon.prefix: "playlist"; action2: "tool/playlist"
            tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
        }
    }
    Component.onCompleted: {
        B.Settings.open(app.name)
        toggler.checked = B.Settings.getBool("toggled", false)
        B.Settings.close()
    }
    Component.onDestruction: {
        B.Settings.open(app.name)
        B.Settings.set("toggled", toggler.checked)
        B.Settings.close()
    }
}

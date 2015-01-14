import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0

AppWithDock {
    id: app
    readonly property real margin: 5
    readonly property Engine engine: App.engine
    Component {
        id: slider
        SliderStyle {
            readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
            groove: Item {
                Rectangle {
                    height: 5; anchors.centerIn: parent; border { color: "#999"; width: 1 }
                    width: parent.width
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#333"}
                        GradientStop {position: 1.0; color: "#bbb"}
                    }
                    Rectangle {
                        height: parent.height; width: parent.width*ratio
                        border { color: "#6ad"; width: 1 }
                        gradient: Gradient {
                            GradientStop {position: 0.0; color: "#fff"}
                            GradientStop {position: 1.0; color: "#ccc"}
                        }
                    }
                }

            }
            handle: Rectangle {
                id: handle
                width: 9; height: 9; radius: 2
                anchors.verticalCenter: parent.verticalCenter
                border.width: control.pressed ? 2 : 1
                border.color: control.pressed || control.hovered ? "#6ad" : "#5c5c5c"
                property Gradient dark: Gradient {
                    GradientStop { position: 0.0; color: "#aaa"}
                    GradientStop { position: 1.0; color: "#999"}
                }
                property Gradient bright: Gradient {
                    GradientStop { position: 0.0; color: "#fff"}
                    GradientStop { position: 1.0; color: "#ccc"}
                }
                gradient: control.pressed || control.hovered ? bright : dark
            }
        }
    }

    controls: Column {
        id: controls; width: parent.width;
        Rectangle {
            id: line
            width: parent.width; height: 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#444" }
                GradientStop { position: 0.5; color: "#888" }
                GradientStop { position: 1.0; color: "#aaa" }
            }
        }
        Image {
            id: bg
            readonly property alias h: bg.height
            source: "bg.png"
            width: parent.width; height: 35
            RowLayout {
                anchors.margins: 2; anchors.fill: parent; spacing: 1
                FramedButton { id: pause; width: height; height: parent.height; action: "play/pause"; icon: engine.playing ? "pause.png" : "play.png" }
                Grid {
                    id: grid; columns: 2; width: h*2; readonly property real h: pause.height/2
                    FramedButton { id: backward; width: grid.h; height: grid.h; action: "play/seek/backward1"; icon: "backward.png" }
                    FramedButton { id: forward; width: grid.h; height: grid.h; action: "play/seek/forward1"; icon: "forward.png" }
                    FramedButton { id: previous; width: grid.h; height: grid.h; action: "play/prev"; icon: "previous.png" }
                    FramedButton { id: next; width: grid.h; height: grid.h; action: "play/next"; icon: "next.png" }
                }
                Column {
                    id: right
                    Layout.fillWidth: true
                    height: parent.height
                    spacing: 2
                    Rectangle {
                        id: panel
                        width: parent.width; height: 20; radius: 3; border {width: 1; color: "#aaa"}
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#111" }
                            GradientStop { position: 0.1; color: "#6ad" }
                            GradientStop { position: 0.8; color: "#6ad" }
                            GradientStop { position: 1.0; color: "#fff" }
                        }
                        RowLayout {
                            anchors.margins: 3; anchors.fill: parent; spacing: 0
                            Text {
                                id: medianumber
                                verticalAlignment: Text.AlignVCenter
                                text: "[%1/%2](%3) ".arg(App.playlist.loaded+1).arg(App.playlist.count).arg(engine.stateText)
                                font { pixelSize: 11; family: Util.monospace }
                            }
                            Text {
                                id: medianame
                                Layout.fillWidth: true
                                text: engine.media.name; elide: Text.ElideMiddle
                                font { pixelSize: 11; family: Util.monospace }
                                verticalAlignment: Text.AlignVCenter
                            }
                            TimeText { textColor: "black"; font.pixelSize: 11; msecs: engine.time }
                            TimeText { textColor: "black"; font.pixelSize: 11; text: "/" }
                            TimeText { textColor: "black"; font.pixelSize: 11; msecs: engine.end }
                        }
                    }
                    RowLayout {
                        width: parent.width; spacing: 1; height: 10
                        TimeSlider { id: timeslider; style: slider; Layout.fillWidth: true }
                        FramedButton {
                            id: mute; width: height; height: parent.height; action: "audio/volume/mute"
                            icon: engine.muted ? "speaker-off.png" : "speaker-on.png"
                        }
                        VolumeSlider { id: volumeslider; width: 70; style: slider }
                    }
                }
            }
        }
    }
}

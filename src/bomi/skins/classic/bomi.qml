import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    readonly property real margin: 5
    readonly property QtObject engine: B.App.engine
    Component {
        id: slider
        SliderStyle {
            groove: Item {
                Rectangle {
                    height: 5; anchors.centerIn: parent; border { color: "#999"; width: 1 }
                    width: parent.width
                    gradient: Gradient {
                        GradientStop {position: 0.0; color: "#333"}
                        GradientStop {position: 1.0; color: "#bbb"}
                    }
                    Rectangle {
                        height: parent.height; width: parent.width*control.rate
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

    bottomControls: Column {
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
                FramedButton { id: pause; width: height; height: parent.height; action: "play/play-pause"; icon.source: engine.playing ? "pause.png" : "play.png" }
                Grid {
                    id: grid; columns: 2; width: h*2; readonly property real h: pause.height/2
                    FramedButton { id: backward; width: grid.h; height: grid.h; action: "play/seek/backward1"; icon.source: "backward.png" }
                    FramedButton { id: forward; width: grid.h; height: grid.h; action: "play/seek/forward1"; icon.source: "forward.png" }
                    FramedButton { id: previous; width: grid.h; height: grid.h; action: "play/prev"; icon.source: "previous.png" }
                    FramedButton { id: next; width: grid.h; height: grid.h; action: "play/next"; icon.source: "next.png" }
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
                            B.Text {
                                id: medianumber
                                content: "[%1/%2](%3) ".arg(B.App.playlist.loaded+1).arg(B.App.playlist.count).arg(engine.stateText)
                                textStyle.font.pixelSize: 11
                            }
                            B.Text {
                                id: medianame
                                width: undefined; Layout.fillWidth: true
                                content: engine.media.name;
                                textStyle: medianumber.textStyle
                                onTextStyleChanged: { textStyle.elide = Text.ElideMiddle }
                            }
                            B.TimeDuration { textStyle: medianumber.textStyle }
                        }
                    }
                    RowLayout {
                        width: parent.width; spacing: 1; height: 10
                        B.TimeSlider { id: timeslider; style: slider; Layout.fillWidth: true }
                        FramedButton {
                            id: mute; width: height; height: parent.height; action: "audio/volume/mute"
                            icon.source: engine.muted ? "speaker-off.png" : "speaker-on.png"
                        }
                        B.VolumeSlider { id: volumeslider; width: 70; style: slider }
                    }
                }
            }
        }
    }
}

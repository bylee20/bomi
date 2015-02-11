import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    readonly property QtObject engine: B.App.engine

    SystemPalette { id: palette }

    Component.onCompleted: {
        var orig = player.showOsdFunc
        player.showOsdFunc = function(msg) {
            osdTimer.running = false
            text.text = msg
            if (B.App.window.fullscreen)
                orig(msg)
            osdTimer.running = true
        }
    }

    Timer {
        id: osdTimer; repeat: false
        interval: B.App.theme.osd.message.duration
        onTriggered: text.text = ""
    }

    controls: Item {
        width: parent.width; height: top.height + bottom.height
        Rectangle {
            id: top
            width: parent.width
            height: layout.height
            color: palette.window
            anchors.bottom: bottom.top
            RowLayout {
                id: layout
                width: parent.width
                height: implicitHeight
                spacing: 0
                ToolButton {
                    id: prev
                    iconName: "media-skip-backward"
                    onClicked: B.App.execute("play/prev")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ToolButton {
                    id: play
                    iconName: engine.playing ? "media-playback-pause" : "media-playback-start"
                    onClicked: B.App.execute("play/pause")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ToolButton {
                    id: stop
                    iconName: "media-playback-stop"
                    onClicked: B.App.execute("play/stop")
                    anchors.verticalCenter: parent.verticalCenter
                }
                ToolButton {
                    id: next
                    iconName: "media-skip-forward"
                    onClicked: B.App.execute("play/next")
                    anchors.verticalCenter: parent.verticalCenter
                }
                B.TimeSlider {
                    id: timeslider; Layout.fillWidth: true;
                    anchors.verticalCenter: parent.verticalCenter
                }
                ToolButton {
                    id: volume
                    iconName: {
                        if (engine.muted)
                            return "audio-volume-muted"
                        if (engine.volume < 30)
                            return "audio-volume-low"
                        if (engine.volume < 80)
                            return "audio-volume-medium"
                        return "audio-volume-high"
                    }
                    onClicked: B.App.execute("audio/volume/mute")
                    anchors.verticalCenter: parent.verticalCenter
                }

                B.VolumeSlider {
                    id: volumeslider; width: 80;
                    anchors.verticalCenter: parent.verticalCenter
                }

                ToolButton {
                    id: full
                    iconName: B.App.window.fullscreen ? "view-restore" : "view-fullscreen"
                    onClicked: B.App.execute("window/full")
                }
            }
        }
        Rectangle {
            id: bottom
            color: "black"
            width: parent.width
            height: timeText.implicitHeight + 4
            anchors.bottom: parent.bottom
            Item {
                anchors.fill: parent
                anchors.margins: 2
                Text {
                    id: text
                    width: parent.width
                    height: parent.height
                    color: "white"
                }
                B.TimeDuration {
                    anchors.right: parent.right
                    id: timeText
                    font: text.font
                    color: "white"

                }
            }
        }

    }
}

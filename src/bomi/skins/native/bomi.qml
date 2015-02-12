import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    readonly property QtObject engine: B.App.engine
    property bool fs: B.App.window.fullscreen

    SystemPalette { id: palette }

    Component.onCompleted: {
        var orig = player.showOsdFunc
        player.showOsdFunc = function(msg) {
            osdTimer.running = false
            text.text = msg
            if (fs)
                orig(msg)
            osdTimer.running = true
        }
        text.text = engine.media.name
    }

    Timer {
        id: osdTimer; repeat: false
        interval: B.App.theme.osd.message.duration
        onTriggered: text.text = engine.media.name
    }
    Connections { target: engine.media; onNameChanged: text.text = target.name }

    controls: Item {
        width: parent.width; height: top.height + bottom.height
        Rectangle {
            id: top
            width: parent.width; height: layout.height + 10
            anchors.bottom: bottom.top; color: palette.window
            RowLayout {
                id: layout; spacing: 0
                width: parent.width; height: implicitHeight
                anchors.verticalCenter: parent.verticalCenter
                MediaButton { icon: "media-skip-backward"; action: "play/prev" }
                MediaButton {
                    icon: "media-playback-" + (engine.playing ? "pause" : "start")
                    action: "play/pause"
                }
                MediaButton { icon: "media-playback-stop"; action: "play/stop" }
                MediaButton { icon: "media-skip-forward"; action: "play/next" }
                B.TimeSlider {
                    id: timeslider; Layout.fillWidth: true;
                    anchors.verticalCenter: parent.verticalCenter
                }
                MediaButton {
                    id: volume
                    icon: {
                        if (engine.muted)
                            return "audio-volume-muted"
                        if (engine.volume < 30)
                            return "audio-volume-low"
                        if (engine.volume < 80)
                            return "audio-volume-medium"
                        return "audio-volume-high"
                    }
                    action: "audio/volume/mute"
                    checkable: true; checked: engine.muted
                }

                B.VolumeSlider {
                    id: volumeslider; width: 80;
                    anchors.verticalCenter: parent.verticalCenter
                }

                MediaButton {
                    icon: "view-" + (fs ? "restore" : "fullscreen")
                    action: "window/full"; checkable: true; checked: fs
                }
            }
        }
        Rectangle {
            id: bottom
            color: "black"; anchors.bottom: parent.bottom
            width: parent.width; height: timeText.implicitHeight + 4
            Item {
                anchors { fill: parent; margins: 2 }
                Text {
                    id: text; color: "white"; elide: Text.ElideRight
                    anchors.fill: parent; anchors.rightMargin: timeText.width
                }
                B.TimeDuration {
                    id: timeText; anchors.right: parent.right
                    font: text.font; color: text.color
                }
            }
        }

    }
}

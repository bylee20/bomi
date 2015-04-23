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
            text.content = msg
            if (fs)
                orig(msg)
            osdTimer.running = true
        }
        text.content = engine.media.name
    }

    Timer {
        id: osdTimer; repeat: false
        interval: B.App.theme.osd.message.duration
        onTriggered: text.content = engine.media.name
    }

    Connections { target: engine.media; onNameChanged: text.content = target.name }

    bottomControls: Item {
        width: parent.width; height: top.height + (fs ? 0 : bottom.height)
        Rectangle {
            id: top
            width: parent.width; height: layout.height + 10
            anchors.bottom: bottom.top; color: palette.window
            RowLayout {
                id: layout; spacing: 0
                width: parent.width - (fs ? (right.width + 10) : 0); height: implicitHeight
                anchors.verticalCenter: parent.verticalCenter
                MediaButton { icon: "media-skip-backward"; action: "play/prev" }
                MediaButton {
                    icon: "media-playback-" + (engine.playing ? "pause" : "start")
                    action: "play/play-pause"
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
                        if (engine.volume < 0.3)
                            return "audio-volume-low"
                        if (engine.volume < 0.8)
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

                MediaButton { icon: "audio-x-generic"; action: "audio/track" }

                MediaButton { icon: "text-x-generic"; action: "subtitle/track" }

                MediaButton {
                    icon: "view-" + (fs ? "restore" : "fullscreen")
                    action: "window/full"; checkable: true; checked: fs
                }

                Item { visible: fs; width: 5; Layout.fillHeight: true }
            }
            Rectangle {
                id: right
                color: "black"; visible: fs; width: fs ? (timeText2.implicitWidth + 10) : 0
                anchors { top: parent.top; bottom: parent.bottom; right: parent.right; margins: 5 }
                B.TimeDuration {
                    id: timeText2
                    anchors.centerIn: parent
                    height: parent.height
                    onMsecChanged: timeText.msec = msec
                    onRemainingChanged: timeText.remaining = remaining
                    textStyle {
                        color: "white"; font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Rectangle {
            id: bottom
            visible: !fs
            color: "black"; anchors.bottom: parent.bottom
            width: parent.width;
            height: fs ? 0 : (timeText.implicitHeight + 4)
            Item {
                anchors { fill: parent; margins: 2 }
                B.Text {
                    id: text;
                    anchors { fill: parent; rightMargin: timeText.width }
                    textStyle {
                        elide: Text.ElideRight; color: "white";
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                B.TimeDuration {
                    id: timeText; anchors.right: parent.right
                    onMsecChanged: timeText2.msec = msec
                    onRemainingChanged: timeText2.remaining = remaining
                    textStyle { color: "white"; verticalAlignment: Text.AlignVCenter }
                }
            }
        }
    }

    B.Text { id: fmt; visible: false }
}

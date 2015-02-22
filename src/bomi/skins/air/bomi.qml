import QtQuick 2.0
import QtQuick.Controls.Styles 1.0
import QtQuick.Layouts 1.0
import bomi 1.0 as B

B.BaseApp {
    id: app
    name: "net.xylosper.air"
    minimumSize: Qt.size(450, 200)

    readonly property QtObject engine: B.App.engine

    B.Player {
        id: playerItem
        anchors.fill: parent
        bottomPadding: bottomItem.height
    }

    player: playerItem

    states: State {
        name: "hidden"; when: !B.App.window.mouse.cursor
        PropertyChanges { target: bottomItem; anchors.bottomMargin: -bottomItem.height }
    }

    transitions: Transition {
        reversible: true; to: "hidden"
        NumberAnimation { target: bottomItem; property: "anchors.bottomMargin"; duration: 200 }
    }

    MouseArea {
        id: bottomItem
        width: parent.width; height: 56; anchors.bottom: parent.bottom
        hoverEnabled: true
        onContainsMouseChanged: B.App.window.mouse.hidingCursorBlocked = containsMouse
        Component.onCompleted: B.App.registerToAccept(bottomItem, B.App.DoubleClickEvent)

        Rectangle {
            id: bottomBoundary; anchors.top: parent.top
            width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.5)
        }

        Rectangle {
            anchors { top: bottomBoundary.bottom; bottom: parent.bottom }
            width: parent.width; color: Qt.rgba(0, 0, 0, 0.5)

            Component {
                id: sliderStyle
                SliderStyle {
                    groove: Rectangle {
                        height: control.height; radius: height*0.5
                        border { color: Qt.rgba(1, 1, 1, 0.5); width: 1 }
                        anchors.verticalCenter: parent.verticalCenter
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(0.1, 0.1, 0.1, 0.2) }
                            GradientStop { position: 1.0; color: Qt.rgba(0.6, 0.6, 0.6, 0.2) }
                        }
                        Rectangle {
                            width: parent.width*control.rate; height: parent.height
                            color: Qt.rgba(1, 0, 0, 0.5); radius: parent.radius
                            border { color: Qt.rgba(1, 1, 1, 0.01); width: 1 }
                        }
                    }
                    handle: Item { }
                }
            }

            RowLayout {
                id: sliders;
                anchors {
                    top: parent.top; topMargin: 3
                    left: parent.left; leftMargin: 10
                    right: parent.right; rightMargin: anchors.leftMargin
                }
                height: timeSlider.height; spacing: 10
                B.TimeSlider {
                    id: timeSlider; bind: timeText; style: sliderStyle
                    Layout.fillWidth: true; height: 8
                    markerStyle: B.Button {
                        size: 4; y: (control.height - height)*0.5
                        background { radius: width*0.5; color: Qt.rgba(1, 1, 1, 0.8) }
                        tooltip: chapter.name; delay: 0
                    }
                }

                B.TimeDuration {
                    id: timeText
                    property bool showLeftTime: true
                    textStyle {
                        color: "white"; font.pixelSize: 12
                        style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, 0.5)
                    }
                    spacing: 1; opacity: 0.8
                    duration: showLeftTime ? (engine.time - engine.end) : engine.end

                    B.Button {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: {
                            if (mouse.button & Qt.RightButton) {
                                timeText.showLeftTime = !timeText.showLeftTime
                            } else {
                                timeText.msec = !timeText.msec
                            }
                        }
                    }
                }

                B.VolumeSlider {
                    style: sliderStyle; width: 80; height: timeSlider.height
                }
            }

            Item {
                anchors {
                    top: sliders.bottom; topMargin: 0
                    bottom: parent.bottom; bottomMargin: sliders.y
                }
                width: parent.width

                Row {
                    id: mediaButtons
                    opacity: 0.8; spacing: 5
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left; leftMargin: sliders.anchors.leftMargin
                    }

                    ImageButton {
                        id: prev
                        icon.source: "prev.png"; size: 24; action: "play/prev"
                    }

                    ImageButton {
                        id: backward
                        icon.source: "backward.png"; size: 24
                        action: "play/seek/backward1"; action2: "play/chapter/prev"
                    }
                    ImageButton {
                        id: play
                        icon.source: engine.playing ? "pause.png" : "play.png"
                        action: "play/pause"; size: 28
                    }
                    ImageButton {
                        id: forward
                        icon.source: "forward.png"; size: 24
                        action: "play/seek/forward1"; action2: "play/chapter/next"
                    }

                    ImageButton {
                        id: next
                        icon.source: "next.png"; size: 24; action: "play/next"
                    }
                }

                IconTextButton {
                    id: playlist
                    anchors { left: mediaButtons.right; leftMargin: 20 }
                    icon.source: "pl.png"; action: "tool/playlist/toggle"
                    readonly property QtObject pl: B.App.playlist
                    text.content: '[' + B.Format.listNumber(pl.currentNumber, pl.length) + ']'
                }

                B.Text {
                    textStyle {
                        color: "white"; elide: Text.ElideRight
                        style: Text.Outline; styleColor: Qt.rgba(0, 0, 0, 0.5)
                        font.pixelSize: 14; verticalAlignment: Text.AlignVCenter
                    }
                    anchors {
                        left: playlist.right; leftMargin: 5
                        right: toolButtons.left; rightMargin: 20
                        verticalCenter: playlist.verticalCenter
                    }
                    opacity: 0.8
                    content: B.App.engine.media.name
                }

                Row {
                    id: toolButtons; spacing: 10
                    anchors {
                        right: parent.right; rightMargin: 15
                        verticalCenter: parent.verticalCenter
                    }
                    IconTextButton {
                        icon.source: "audio.png"; action: "audio/track"
                        text.content: formatTrackNumber(engine.audio)
                    }
                    IconTextButton {
                        icon.source: "sub.png"; action: "subtitle/track"
                        text.content: formatTrackNumber(engine.subtitle)
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        B.Settings.open(name)
        timeText.msec = B.Settings.getBool("time-msec", false)
        timeText.showLeftTime = B.Settings.getBool("time-left", true)
        B.Settings.close()
    }
    Component.onDestruction: {
        B.Settings.open(name)
        B.Settings.set("time-msec", timeText.msec)
        B.Settings.set("time-left", timeText.showLeftTime )
        B.Settings.close()
    }
}

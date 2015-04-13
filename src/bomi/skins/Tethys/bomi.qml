// CREDIT: designed by Ivan(Kotus works)
//         https://plus.google.com/u/1/117118228830713086299/posts

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithDock {
    id: app
    name: "net.xylosper.Tethys"

    readonly property int s_thickness: 4
    readonly property int s_shadow: 1
    property bool compact: false

    overlaps: true
    trackingMinY: height - Math.max(height * 0.35, bottomControls.height)

    Component {
        id: sliders
        SliderStyle {
            groove: Item {
                Rectangle {
                    color: Qt.rgba(0, 0, 0, 0.4)
                    width: parent.width; height: s_thickness; y: s_shadow * 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: s_shadow
                }
                Rectangle {
                    width: parent.width; height: s_thickness; y: s_shadow
                    radius: height * 0.5; color: "#929292"
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        width: parent.width * control.rate; height: parent.height
                        radius: parent.radius; color: "#d73d48"
                    }
                }
            }
            handle: Image {
                width: control.height; height: width
                source: control.hpressed ? "handle-pressed.png"
                      : control.hhovered ? "handle-hovered.png" : "handle.png"
                x: (control.rate - 0.5) * width
            }
        }
    }

    Component {
        id: rightButtons
        Row {

            B.Button {
                size: 24; icon.prefix: "playlist"
                anchors.verticalCenter: parent.verticalCenter
                action: "tool/playlist/toggle"; action2: "tool/playlist"
            }
            B.Button {
                size: 24; icon.prefix: "subscale"
                anchors.verticalCenter: parent.verticalCenter
                action: "subtitle/scale/increase"; action2: "subtitle/scale/decrease"
            }
            B.Button {
                size: 24; icon.prefix: "sub"
                anchors.verticalCenter: parent.verticalCenter
                action: "subtitle/track/next"; action2: "subtitle/track"
            }
            B.Button {
                size: 24; icon.prefix: "audio"
                anchors.verticalCenter: parent.verticalCenter
                action: "audio/track/next"; action2: "audio/track"
            }
            B.Button {
                size: 24; icon.prefix: "fs"; action: "window/full"
                checked: B.App.window.fullscreen
                anchors.verticalCenter: parent.verticalCenter
            }
            B.Button {
                background.color: "white";
                background.border {
                    color: "black"
                    width: 1
                }
                size: 8
                onClicked: { compact = !compact }
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Component {
        id: mediaButtonComponent
        Row {
            spacing: 0
            B.Button {
                width: 32; height: 24
                icon.prefix: "backward"; action: "play/seek/backward1"
                anchors.verticalCenter: parent.verticalCenter
            }
            B.Button {
                size: bigSize; action: "play/pause"
                icon.prefix: engine.playing ? "pause" : "play"
                anchors.verticalCenter: parent.verticalCenter
            }
            B.Button {
                width: 32; height: 24
                icon.prefix: "forward"; action: "play/seek/forward1"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Component {
        id: volumeIcon
        Item {
            width: 24; height: 24
            Image { source: engine.muted ? "volume-muted-shadow.png" : "volume-shadow.png" }
            Image { source: "volume-blur.png"; visible: volumeButton.hovered }
            B.Button {
                id: volumeButton
                size: 24; action: "audio/volume/mute"
                icon.source: engine.muted ? "volume-mute.png"
                           : engine.volume < 0.05 ? "volume-0.png"
                           : engine.volume < 0.4 ? "volume-low.png"
                           : engine.volume < 0.8 ? "volume-high.png" : "volume-100.png"
            }
            Image {
                visible: volumeButton.pressed
                source: engine.muted ? "volume-muted-pressed.png" : "volume-pressed.png"
            }
        }
    }

    Component {
        id: timeslider
        B.TimeSlider {
            style: sliders; bind: timeDuration
            width: parent.width; height: parent.height
            markerStyle: B.Button {
                readonly property bool emph: hovered || pressed
                size: 8; z: emph ? 1e10 : -1
                y: control.height - 2*(pressed ? 1 : hovered ? -1 : 0) - 10
                icon.source: "marker.png"
                tooltip: chapter.name; delay: 0
            }
        }
    }

    Component {
        id: normalComponent
        Item {
            implicitHeight: 80
            anchors {
                fill: parent
                leftMargin: 24; rightMargin: anchors.leftMargin
                topMargin: 10
            }

            Loader {
                readonly property var timeDuration: time
                sourceComponent: timeslider
                width: parent.width; height: 24
            }

            Row {
                id: mediaButtons; height: 44
                anchors { bottom: parent.bottom; bottomMargin: 6; left: parent.left }
                Loader {
                    readonly property int bigSize: 44
                    anchors.verticalCenter: parent.verticalCenter
                    sourceComponent: mediaButtonComponent
                }

                MouseArea {
                    id: volumeArea
                    width: volume.width + 40; height: 40
                    anchors.verticalCenter: parent.verticalCenter
                    hoverEnabled: true
                    Loader {
                        sourceComponent: volumeIcon
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Item {
                        id: volumeBox; width: 19; height: parent.height; clip:true
                        B.VolumeSlider {
                            id: volume; style: sliders
                            x: 26; width: Math.min(app.width * 0.09, 80); height: 14
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    property bool changing: false
                    states: State {
                        name: "show"; when: volumeArea.containsMouse || volumeArea.changing
                        PropertyChanges { target: volumeBox; width: volume.width + 40 }
                    }
                    Timer {
                        id: volumeTimer
                        interval: 1000; repeat: false
                        onTriggered: volumeArea.changing = false
                    }

                    Connections {
                        target: engine
                        onVolumeChanged: {
                            volumeArea.changing = true
                            volumeTimer.stop(); volumeTimer.start()
                        }
                    }

                    transitions: Transition {
                        reversible: true; to: "show"
                        NumberAnimation { target: volumeBox; property: "width"; duration: 200 }
                    }
                }
            }

            B.TimeDuration {
                id: time; spacing: 4
                anchors.verticalCenter: mediaButtons.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                textStyle { color: "white"; style: Text.Raised; styleColor: "black" }
            }

            Loader {
                sourceComponent: rightButtons
                height: 44
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 6
                    right: parent.right
                }
            }
        }
    }

    Component {
        id: compactComponent
        Rectangle {
            anchors.fill: parent
            implicitHeight: 36
            readonly property real bs: 0.6 // button scale
            readonly property real ss: 24 * bs;
            color: Qt.rgba(0, 0, 0, 0.6)
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: anchors.leftMargin
                Loader {
                    readonly property int bigSize: 32
                    anchors.verticalCenter: parent.verticalCenter
                    sourceComponent: mediaButtonComponent
                }
                B.TimeDuration {
                    id: td; spacing: 2
                    height: parent.height; width: contentWidth
                    anchors.verticalCenter: parent.verticalCenter
                    textStyle {
                        color: "white"; style: Text.Raised; styleColor: "black"
                        font.pixelSize: 9;
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { width: 3 }

                Loader {
                    readonly property var timeDuration: td
                    anchors.verticalCenter: parent.verticalCenter
                    width: 100; height: 14
                    sourceComponent: timeslider
                    Layout.fillWidth: true
                }

                Loader {
                    sourceComponent: volumeIcon
                    anchors.verticalCenter: parent.verticalCenter
                }

                B.VolumeSlider {
                    id: volumeCompact; style: sliders
                    width: 70; height: 14
                    anchors.verticalCenter: parent.verticalCenter
                    visible: app.width > 500
                }

                Loader {
                    sourceComponent: rightButtons
                    height: 24
                }
            }
        }
    }

    bottomControls: Rectangle {
        width: parent.width; height: controlLoader.implicitHeight
        gradient: Gradient {
            GradientStop {
                position: 0.0; color: Qt.rgba(0, 0, 0, 0)
            }
            GradientStop {
                position: 0.9; color: Qt.rgba(0, 0, 0, 0.82)
            }
        }
        Loader {
            id: controlLoader
            anchors.fill: parent
            sourceComponent: compact ? compactComponent : normalComponent
        }
    }

    Component.onCompleted: {
        B.Settings.open(app.name)
        compact = B.Settings.getBool("compact", false)
        B.Settings.close(app.name)
    }
    Component.onDestruction: {
        B.Settings.open(app.name)
        B.Settings.set("compact", compact)
        B.Settings.close(app.name)
    }
}

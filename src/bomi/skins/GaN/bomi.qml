import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithFloating {
    id: skin; name: "net.xylosper.bomi.GaN"
    onWidthChanged: controls.width = width < 550 ? 400 : 550
    readonly property QtObject engine: B.App.engine
    controls: Item {
        width: 550; height: topBox.height + timeslide.height + bottomBox.height + 5
        Rectangle {
            id: bg; anchors.fill: parent; border { color: "white"; width: 1 } radius: 10; color: Qt.rgba(0, 0, 0, 0.5)
            Column {
                clip: true; anchors { fill: parent; margins: 1 }
                Item {
                    id: topBox; width: parent.width; height: 26
                    Item {
                        anchors { fill: parent; topMargin: 4.5; leftMargin: 10; rightMargin: 10 }
                        TextButton {
                            id: audio
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            icon.prefix: "audios"; action: "audio/track/next"; action2: "audio/track"
                            text.content: formatTrackNumber(engine.audio)
                        }
                        TextButton {
                            anchors { left: audio.right; verticalCenter: parent.verticalCenter; leftMargin: 5 }
                            icon.prefix: "subs"; action: "subtitle/track/next"; action2: "subtitle/track"
                            text.content: formatTrackNumber(engine.subtitle)
                        }

                        TextButton {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            icon.prefix: "playlist"; action: "tool/playlist/toggle"; action2: "tool/playlist"
                            text.content: B.Format.listNumber(B.App.playlist.loaded+1, B.App.playlist.count)
                        }
                    }
                    Item {
                        anchors { fill: parent; topMargin: 4 }
                        Text {
                            anchors.centerIn: parent; width: parent.width-230; height: parent.height
                            text: engine.media.name; elide: Text.ElideMiddle
                            color: "white"; font { family: "Sans"; pixelSize: 13 }
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                B.TimeSlider {
                    id: timeslide; width: parent.width; height: 10
                    style: SliderStyle {
                        groove: Rectangle {
                            height: 1; anchors.verticalCenter: parent.verticalCenter; color: "white"
                            Rectangle {
                                width: parent.width * control.arate; height: parent.height
                                color: "#0ef"; opacity: 0.8
                            }
                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width*control.rate; height: 10
                                source: "timeslide-fill.png"; fillMode: Image.TileHorizontally
                            }
                        }
                        handle: Item { Image { anchors { centerIn: parent; verticalCenter: parent.verticalCenter } source: "timeslide-handle.png" } }
                    }
                    markerStyle: B.Button {
                        size: 6; z: hovered ? 1e10 : -1
                        icon.prefix: "marker"; tooltip: chapter.name; delay: 0
                        onClicked: control.time = chapter.time
                    }
                }
                Item {
                    id: bottomBox; width: parent.width; height: 40
                    Column {
                        anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
                        TimeText {
                            id: timetext; time: engine.time
                            tooltip: qsTr("Show/Hide milliseconds"); msec: checked
                        }
                        TimeText {
                            id: endtext; time: checked ? (engine.end - engine.time) : engine.end
                            tooltip: qsTr("Toggle end time/left time"); msec: timetext.msec
                        }
                    }

                    Row {
                        spacing: 5; anchors.centerIn: parent; property real size: 24
                        B.Button { size: parent.size; icon.prefix: "prev"; action: "play/prev"; action2: "play/chapter/prev"}
                        B.Button { size: parent.size; icon.prefix: "backward"; action: "play/seek/backward1"; action2: "play/seek/backward2" }
                        B.Button { size: parent.size; icon.prefix: engine.playing ? "pause" : "play"; action: "play/play-pause" }
                        B.Button { size: parent.size; icon.prefix: "forward"; action: "play/seek/forward1"; action2: "play/seek/forward2" }
                        B.Button { size: parent.size; icon.prefix: "next"; action: "play/next";  action2: "play/chapter/next" }
                    }

                    Item {
                        width: 48; height: 22
                        anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 10 }
                        B.VolumeSlider {
                            id: volume; anchors.fill: parent
                            style: SliderStyle {
                                groove: Image {
                                    width: volume.width; height: volume.height; source: "volume-unfill.png"
                                    Item {
                                        clip: true; width: parent.width*engine.volume; height: parent.height
                                        Image { width: volume.width; height: volume.height ;source: "volume-fill.png" }
                                    }
                                }
                                handle: Item {}
                            }
                            B.Button {
                                width: 12; height: 12; checked: engine.muted; action: "audio/volume/mute"
                                icon.source: pressed ? "mute-pressed.png" : (hovered || checked ? "mute-checked.png" : "mute.png")
                            }
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        B.Settings.open(name)
        timetext.checked = B.Settings.getBool("time-checked", false)
        endtext.checked = B.Settings.getBool("end-checked", false)
        B.Settings.close()
    }
    Component.onDestruction: {
        B.Settings.open(name)
        B.Settings.set("time-checked", timetext.checked)
        B.Settings.set("end-checked", endtext.checked)
        B.Settings.close()
    }
}

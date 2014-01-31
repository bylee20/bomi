import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.AppWithFloating {
	id: skin; name: "net.xylosper.cmplayer.skin.GaN"
	onWidthChanged: controls.width = width < 550 ? 400 : 550
	controls: Item {
		width: 550; height: topBox.height + timeslide.height + bottomBox.height + 5
		Rectangle {
			id: bg; anchors.fill: parent; border { color: "white"; width: 1 } radius: 10; color: Qt.rgba(0, 0, 0, 0.5)
			Column {
				clip: true; anchors { fill: parent; margins: 1 }
				Item {
					id: topBox; width: parent.width; height: 26
					RowLayout {
						anchors { fill: parent; topMargin: 4.5; leftMargin: 10; rightMargin: 10 } spacing: 2
						Button {
							id: audioTrackIcon; bind: audioTrackText; width: 20; height: 12; iconName: "audios"
							action: "audio/track/next"; action2: "audio/track"
							tooltip: makeToolTip(qsTr("Next Audio Track"), qsTr("Show Audio tracks"))
						}
						TimeText {
							id: audioTrackText; bind: audioTrackIcon; width: textWidth; height: 12
							text: engine.audioTrack.currentText + "/" + engine.audioTrack.countText
						}
						Item { width: 2; height: 1 }
						Button {
							id: subTrackIcon; bind: subTrackText; width: 20; height: 12; iconName: "subs"
							action: "subtitle/track/next"; action2: "subtitle/track"
							tooltip: makeToolTip(qsTr("Next Subtitle"), qsTr("Show Subtitles"))
						}
						TimeText {
							id: subTrackText; bind: subTrackIcon; width: textWidth; height: 12
							text: engine.subtitleTrack.currentText + "/" + engine.subtitleTrack.countText
						}
						Item { Layout.fillWidth: true }
						Button {
							id: playlistIcon; bind: playlistText; width: 20; height: 12; iconName: "playlist"
							action: "tool/playlist/toggle"; action2: "tool/playlist"
							tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
						}
						TimeText {
							id: playlistText; bind: playlistIcon; width: textWidth; height: 12
							text: (playlist.loaded+1).toString() + "/" + playlist.count
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

				Skin.TimeSlider {
					id: timeslide; width: parent.width; height: 10
					style: SliderStyle {
						groove: Rectangle {
							height: 1; anchors.verticalCenter: parent.verticalCenter; color: "white"
							Image {
								anchors.verticalCenter: parent.verticalCenter
								readonly property real ratio: control.range > 0 ? (control.value - control.min)/control.range : 0.0
								width: parent.width*ratio; height: 10
								source: "timeslide-fill.png"; fillMode: Image.TileHorizontally
							}
						}
						handle: Item { Image { anchors { centerIn: parent; verticalCenter: parent.verticalCenter } source: "timeslide-handle.png" } }
					}
					Component {
						id: chapterMarker
						Button {
							property int chapter: -2
							size: 6; y: (timeslide.height*0.5-6-1)
							x: timeslide.width*(engine.chapter.time(chapter) - engine.begin)/(engine.duration)-3
							iconName: "marker"; tooltip: engine.chapter.name(chapter); tooltipDelay: 0
							onClicked: engine.seek(engine.chapter.time(chapter))
						}
					}
					property var markers: []
					Connections { target: engine; onChapterChanged: timeslide.generateChapters() }
					function generateChapters() {
						var i;
						for (i=0; i<markers.length; ++i)
							markers[i].destroy()
						markers = []
						var chapter = engine.chapter
						for (i=0; i<chapter.count; ++i)
							markers.push(chapterMarker.createObject(timeslide, { "chapter": i }));
					}
				}
				Item {
					id: bottomBox; width: parent.width; height: 40
					Column {
						anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
						TimeText {
							id: timetext; msecs: engine.time; showMSecs: checked
							tooltip: qsTr("Show/Hide milliseconds")
						}
						TimeText {
							id: endtext; msecs: checked ? (engine.end - engine.time) : engine.end
							showMSecs: timetext.checked; tooltip: qsTr("Toggle end time/left time")
						}
					}

					Row {
						spacing: 5; anchors.centerIn: parent; property real size: 24
						Button { size: parent.size; iconName: "prev"; action: "play/prev"; action2: "play/chapter/prev"}
						Button { size: parent.size; iconName: "backward"; action: "play/seek/backward1"; action2: "play/seek/backward2" }
						Button { size: parent.size; iconName: engine.running ? "pause" : "play"; action: "play/pause" }
						Button { size: parent.size; iconName: "forward"; action: "play/seek/forward1"; action2: "play/seek/forward2" }
						Button { size: parent.size; iconName: "next"; action: "play/next";  action2: "play/chapter/next" }
					}

					Item {
						width: 48; height: 22
						anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 10 }
						Skin.VolumeSlider {
							id: volume; anchors.fill: parent
							style: SliderStyle {
								groove: Image {
									width: volume.width; height: volume.height; source: "volume-unfill.png"
									Item {
										clip: true; width: parent.width*engine.volume*1e-2; height: parent.height
										Image { width: volume.width; height: volume.height ;source: "volume-fill.png" }
									}
								}
								handle: Item {}
							}
							Skin.Button {
								width: 12; height: 12; checked: engine.muted; action: "audio/volume/mute"
								icon: pressed ? "mute-pressed.png" : (hovered || checked ? "mute-checked.png" : "mute.png")
							}
						}
					}
				}
			}
		}
	}
	Component.onCompleted: {
		Core.Settings.open(skin.name)
		timetext.checked = Core.Settings.getBool("time-checked", false)
		endtext.checked = Core.Settings.getBool("end-checked", false)
		Core.Settings.close()
		timeslide.generateChapters()
	}
	Component.onDestruction: {
		Core.Settings.open(skin.name)
		Core.Settings.set("time-checked", timetext.checked)
		Core.Settings.set("end-checked", endtext.checked)
		Core.Settings.close()
	}
}

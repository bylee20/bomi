import QtQuick 2.0
import CMPlayerSkin 1.0

Rectangle {
	id: main
	readonly property real margin: 5
	Player {
		id: player
		width: parent.width; height: fullScreen ? parent.height : parent.height - controls.height
		onFullScreenChanged: controls.update(); onTick: timetext.secs = (time*1e-3).toFixed(0)
	}
	MouseCatcher {
		id: catcher
		width: parent.width; height: controls.height; anchors.bottom: parent.bottom
		tracking: player.fullScreen; onCatchedChanged: controls.update()
		Column {
			id: controls
			width: parent.width;
			function update() {
				if (!catcher.catched && player.fullScreen)
					hider.start()
				else
					controls.opacity = 1.0
			}
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
				HorizontalLayout {
					anchors {fill: parent} spacing: 1; paddings: 2; bottomPadding: 1; fillers: [right]
					FramedButton { id: pause; width: height; action: "menu/play/pause"; icon: player.playing ? "pause.png" : "play.png" }
					Grid {
						id: grid; columns: 2; width: h*2; readonly property real h: pause.height/2
						FramedButton { id: backward; width: grid.h; height: grid.h; action: "menu/play/backward"; icon: "backward.png" }
						FramedButton { id: forward; width: grid.h; height: grid.h; action: "menu/play/forward"; icon: "forward.png" }
						FramedButton { id: previous; width: grid.h; height: grid.h; action: "menu/play/previous"; icon: "previous.png" }
						FramedButton { id: next; width: grid.h; height: grid.h; action: "menu/play/next"; icon: "next.png" }
					}
					Column {
						id: right
						spacing: 1
						Rectangle {
							id: panel
							width: parent.width
							border {width: 1; color: "#aaa"}
							height: 20
							radius: 3
							gradient: Gradient {
								GradientStop { position: 0.0; color: "#111" }
								GradientStop { position: 0.1; color: "#6ad" }
								GradientStop { position: 0.8; color: "#6ad" }
								GradientStop { position: 1.0; color: "#fff" }
							}
							HorizontalLayout {
								anchors.fill: parent; fillers: [medianame]; paddings: 3; centering: true
								Text {
									id: medianumber
									width: contentWidth; verticalAlignment: Text.AlignVCenter
									text: "[%1/%2](%3) ".arg(playlist.loaded+1).arg(playlist.count).arg(player.stateText)
									font { pixelSize: 11; family: Util.monospace }
								}
								Text {
									id: medianame
									text: player.media.name; elide: Text.ElideMiddle
									font { pixelSize: 11; family: Util.monospace }
									verticalAlignment: Text.AlignVCenter
								}
								Text {
									id: timetext
									width: contentWidth; verticalAlignment: Text.AlignVCenter
									property int secs: 0
									text: "%1/%2".arg(Util.msecToString(secs*1000.0)).arg(Util.msecToString(player.duration))
									font { pixelSize: 11; family: Util.monospace }
								}
							}
						}
						HorizontalLayout {
							width: parent.width; height: pause.height-1-panel.height; fillers: [timeslider]; spacing: 1
							Slider {
								id: timeslider
								onPressed: player.seek(Math.floor(player.duration*target))
								onDragged: player.seek(Math.floor(player.duration*target))
								value: player.time/player.duration
							}
							FramedButton {
								id: mute; width: height; action: "menu/audio/mute"
								icon: player.muted ? "speaker-off.png" : "speaker-on.png"
							}
							Slider {
								id: volumeslider; width: 70; value: player.volume*1e-2
								onPressed: player.setVolume(Math.floor(100.0*target))
								onDragged: player.setVolume(Math.floor(100.0*target))
							}
						}
					}
				}
			}
			NumberAnimation { id: hider; target: controls; property: "opacity"; from: 1.0; to: 0.0; duration: 200}
		}
	}
}

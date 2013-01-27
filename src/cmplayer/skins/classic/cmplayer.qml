import QtQuick 2.0
import CMPlayerCore 1.0
import "qml"

Rectangle {
	id: main
	readonly property real margin: 5
	Player {
		id: player
		anchors {top: parent.top; left: parent.left; right: parent.right; bottom: controls.top }
		infoView: PlayInfoOsd { parent: player; info: player.info }
		info.onTick: { timeline.value = info.time/info.duration; timetext.secs = (info.time*1e-3).toFixed(0) }
		onMessageRequested: { msgosd.text = message; msgosd.show(); }
		onSought: { timeline.show(); }
		Logo { id: logo; anchors.fill: parent; visible: player.info.state == PlayInfo.Stopped }
		TextOsd { id: msgosd; anchors.fill: parent }
		ProgressOsd { id: timeline; anchors.fill: parent}
		info.onFullScreenChanged: {
			player.anchors.bottom = info.fullScreen ? parent.bottom : controls.top
			if (info.fullScreen)
				controls.hide()
			else
				controls.opacity = 1.0;
		}
		info.onMediaChanged: {medianame.text = info.media.name}
	}
	Item {
		id: controls
		height: 40
		anchors {left: parent.left; right: parent.right; bottom: parent.bottom}
		Rectangle {
			id: line
			height: 4
			anchors { top: parent.top; left: parent.left; right: parent.right }
			gradient: Gradient {
				GradientStop { position: 0.0; color: "#000" }
				GradientStop { position: 0.1; color: "#000" }
				GradientStop { position: 0.5; color: "#888" }
				GradientStop { position: 0.6; color: "#000" }
				GradientStop { position: 1.0; color: "#000" }
			}
		}
		Image {
			id: controlsBackground
			source: "bg.png"
			anchors {top: line.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
			Item {
				anchors {fill: parent; topMargin: 0; leftMargin: 2; rightMargin: 2; bottomMargin: 2}
				ImageButton {
					id: playPause
					width: height
					action: "menu/play/pause"
					source: player.info.state == PlayInfo.Playing ? "pause.png" : "play.png"
					anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
				}
				ImageButton {
					id: backward
					width: height
					height: playPause.height/2
					action: "menu/play/backward"
					source: "backward.png"
					anchors { top: parent.top; left: playPause.right }
				}
				ImageButton {
					id: forward
					width: height
					height: playPause.height/2
					action: "menu/play/forward"
					source: "forward.png"
					anchors { top: parent.top; left: backward.right }
				}
				ImageButton {
					id: previous
					width: height
					height: playPause.height/2
					action: "menu/play/previous"
					source: "previous.png"
					anchors { top: backward.bottom; left: playPause.right }
				}
				ImageButton {
					id: next
					width: height
					height: playPause.height/2
					action: "menu/play/next"
					source: "next.png"
					anchors {top: forward.bottom; left: previous.right }
				}
				Rectangle {
					id: panel
					anchors {left: next.right; right: parent.right; top: parent.top; leftMargin: 2}
					border {width: 1; color: "#aaa"}
					height: 20
					radius: 3
					gradient: Gradient {
						GradientStop { position: 0.0; color: "#111" }
						GradientStop { position: 0.1; color: "#6ad" }
						GradientStop { position: 0.8; color: "#6ad" }
						GradientStop { position: 1.0; color: "#fff" }
					}
					Text {
						id: medianumber
						anchors { top: parent.top; left: parent.left; bottom: parent.bottom; leftMargin: 3 }
						width: contentWidth
						text: "[%1/%2](%3)"
						font { pixelSize: 11; family: player.info.monospace }
						verticalAlignment: Text.AlignVCenter
					}
					Text {
						id: medianame
						anchors { top: parent.top; left: medianumber.right; right: timetext.left; bottom: parent.bottom }
						text: "test text"
						elide: Text.ElideMiddle
						font { pixelSize: 11; family: player.info.monospace }
						verticalAlignment: Text.AlignVCenter
					}
					Text {
						id: timetext
						anchors { top: parent.top; right: parent.right; bottom: parent.bottom; rightMargin: 3; leftMargin: 3 }
						width: contentWidth
						property int secs: 0
						text: "%1/%2".arg(player.info.msecToString(secs*1000.0)).arg(player.info.msecToString(player.info.duration))
						font { pixelSize: 11; family: player.info.monospace }
						verticalAlignment: Text.AlignVCenter
					}
				}
				Item {
					anchors {top: panel.bottom; left: panel.left; right: panel.right; bottom: parent.bottom; topMargin: 2}
					Slider {
						id: timeslider
						anchors { verticalCenter: parent.verticalCenter; left: parent.left; right: mute.left; rightMargin: 2 }
						onPressed: player.seek(Math.floor(player.info.duration*target))
						onDragged: player.seek(Math.floor(player.info.duration*target))
						value: player.info.time/player.info.duration
					}
					ImageButton {
						id: mute
						anchors { verticalCenter: parent.verticalCenter; right: volumeslider.left; rightMargin: 2 }
						width: height
						height: 12
						action: "menu/audio/mute"
						source: player.info.muted ? "speaker-off.png" : "speaker-on.png"
					}
					Slider {
						id: volumeslider
						anchors.verticalCenter: parent.verticalCenter
						anchors.right: parent.right
						width: 70
						value: player.info.volume*1e-2
						onPressed: player.setVolume(Math.floor(100.0*target))
						onDragged: player.setVolume(Math.floor(100.0*target))
					}
				}
			}
		}
		function hide() {
			if (!hider.running)
				hider.start()
		}
		NumberAnimation { id: hider; target: controls; property: "opacity"; from: 1.0; to: 0.0; duration: 200; }
	}
	MouseArea {
		anchors.fill: parent
		hoverEnabled: true || player.info.fullScreen
		onPressed: mouse.accepted = false;
		onPositionChanged: {
			if (player.info.fullScreen) {
				if (0.0 <= mouse.x && mouse.x <= width && parent.height - controls.height <= mouse.y && mouse.y <= parent.height)
					controls.opacity = 1.0
				else if (controls.opacity == 1.0)
					controls.hide();
			} else
				controls.opacity = 1.0
			mouse.accepted = false
		}
	}
}

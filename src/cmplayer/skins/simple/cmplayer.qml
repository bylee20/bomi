import QtQuick 2.0
import CMPlayer 1.0
import "qml"

Rectangle {
	id: main
	Player {
		id: player
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: controls.top
		infoView: PlayInfoOsd {
			parent: player
			info: player.info
		}
		TextOsd {
			id: msgosd
			anchors.fill: parent
		}
		ProgressOsd {
			id: timeline
			anchors.fill: parent
		}
		info.onTick: {
			timeline.value = info.time/info.duration
			timetext.secs = (info.time*1e-3).toFixed(0)
		}
		onMessageRequested: {msgosd.text = message; msgosd.show();}
		onSought: {timeline.show();}
		info.onStateChanged: {
			if (info.state == PlayInfo.Stopped)
				logo.visible = true
			else
				logo.visible = false
		}
		info.onFullScreenChanged: {
			anchors.bottom = info.fullScreen ? parent.bottom : controls.top;
			controls.visible = !info.fullScreen
		}

		Logo {
			id: logo
			anchors.fill: parent
		}
	}

	Rectangle {
		id: controls
		visible: !player.info.fullScreen
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		height: 20
		gradient: Gradient {
			GradientStop {position: 0.0; color: "#aaa"}
			GradientStop {position: 0.1; color: "#eee"}
			GradientStop {position: 1.0; color: "#aaa"}
		}
		Item {
			id: playPause
			width: height
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.left: parent.left
			anchors.margins: 3
			Image {
				id: playPauseImage
				anchors.fill: parent
				anchors.margins: 2
				smooth: true
				source: (player.info.state == PlayInfo.Playing) ? "pause.png" : "play.png"
			}
			MouseArea {
				anchors.fill: parent
				hoverEnabled: true
				onEntered: playPauseImage.anchors.margins = 0
				onExited: playPauseImage.anchors.margins = 2
				onPressedChanged: playPauseImage.anchors.margins = pressed ? 2 : 0
				onReleased: {if (containsMouse) player.execute("menu/play/pause")}
			}
		}
		Slider {
			id: timeslider
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: timetext.left
			anchors.left: playPause.right
			anchors.margins: 5
			anchors.leftMargin: 2
			value: player.info.time/player.info.duration
			onPressed: player.seek(Math.floor(player.info.duration*target))
		}
		Text {
			id: timetext
			property int secs: 0
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: volumeslider.left
			width: contentWidth
			text: "%1/%2".arg(player.info.msecToString(secs*1000.0)).arg(player.info.msecToString(player.info.duration))
			font.family: player.info.monospace
			font.pixelSize: 10
			anchors.margins: timeslider.anchors.margins
			verticalAlignment: Text.AlignVCenter
		}
		Slider {
			id: volumeslider
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: parent.right
			anchors.margins: timeslider.anchors.margins
			width: 100
			value: player.info.volume*1e-2
			onPressed: player.setVolume(Math.floor(100.0*target))
			onDragged: player.setVolume(Math.floor(100.0*target))
		}
	}
	MouseArea {
		anchors.fill: parent
		hoverEnabled: player.info.fullScreen
		onPressed: {mouse.accepted = false}
		onPositionChanged: {
			if (player.info.fullScreen) {
				var my = parent.height - mouse.y;
				controls.visible = 0 <= my && my <= controls.height;
			} else
				controls.visible = true
			mouse.accepted = false
		}
	}
}

import QtQuick 2.0
import CMPlayerCore 1.0
import "qml"

Rectangle {
	id: main
	Player {
		id: player
		anchors { top: parent.top; bottom: controls.top; left: parent.left; right: parent.right }

		TextOsd { id: msgosd; anchors.fill: parent }
		onMessageRequested: { msgosd.text = message; msgosd.show(); }

		ProgressOsd { id: timeline;  anchors.fill: parent }
		onTick: { timeline.value = time/duration; timetext.secs = (time*1e-3).toFixed(0) }
		onSought: {timeline.show();}

		onFullScreenChanged: {
			if (fullScreen) { anchors.bottom = parent.bottom; controls.hide(); }
			else { anchors.bottom = controls.top; controls.show() }
		}

		PlayInfoOsd {
			objectName: "playinfo"; parent: player; player: player
			visible: true
		}

		Logo { id: logo; anchors.fill: parent; visible: player.state == Player.Stopped }

		function showSize() { msgosd.text = "%1x%2".arg(width).arg(height); msgosd.show() }
		onHeightChanged: showSize()
		onWidthChanged: {showSize(); playlistcontrol.updateWidth(); playlistcontrol.x = playlistcontrol.dest;}

//playlistview.width = Math.min(width*0.4, playlistview.contentWidth())
		Rectangle {
			id: playlistcontrol
			objectName: "playlist"
			y: 20
			color: "gray"
			opacity: 0.8
			height: parent.height-40
			radius: 10
			visible: false
			onVisibleChanged: if (visible) {updateWidth(); playlistslide.start(); }
			function updateWidth() {
				if (visible) {
					var w = Math.min(parent.width*0.4, playlistview.contentWidth());
					playlistview.width = w
					width = (w+20)*1.5
					dest = player.width - w - 40
				}
			}
			property real dest: 0
			NumberAnimation {
				id: playlistslide;		target: playlistcontrol;		property: "x"
				from: player.width;	to: playlistcontrol.dest;	easing.type: Easing.OutBack
			}
			PlaylistView {
				id: playlistview
				anchors {
					top: parent.top; bottom: parent.bottom; left: parent.left
					topMargin: 10; bottomMargin: 10; leftMargin: 10;
				}

			}
		}

		Rectangle {
			id: historycontrol
			objectName: "history"
			y: 20
			color: "gray"
			opacity: 0.8
			height: parent.height-40
			radius: 10
			visible: false
			width: (historyview.width+20)*1.5
			property real dest: 20-(historycontrol.width - historyview.width)
			onDestChanged: x = dest
			HistoryView {
				id: historyview
				anchors {
					top: parent.top; bottom: parent.bottom; right: parent.right
					topMargin: 10; bottomMargin: 10; rightMargin: 10;
				}
				width: Math.min(player.width*0.4, historyview.contentWidth)
			}
			onVisibleChanged: if (visible) { historyslide.start(); }
			NumberAnimation {
				id: historyslide;		target: historycontrol;		property: "x"
				from: -historycontrol.width;	to: historycontrol.dest;	easing.type: Easing.OutBack
			}
		}
	}

	Rectangle {
		id: controls
		anchors.bottom: parent.bottom; height: 20
		anchors.left: parent.left; anchors.right: parent.right
		gradient: Gradient {
			GradientStop { position: 0.0; color: "#aaa" }
			GradientStop { position: 0.1; color: "#eee" }
			GradientStop { position: 1.0; color: "#aaa" }
		}
		Item {
			id: playPause
			width: height
			anchors.top: parent.top; anchors.bottom: parent.bottom
			anchors.left: parent.left; anchors.margins: 3
			Image {
				id: playPauseImage
				anchors.fill: parent
				anchors.margins: 2
				smooth: true
				source: (player.state == Player.Playing) ? "pause.png" : "play.png"
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
			value: player.time/player.duration
			onPressed: player.seek(Math.floor(player.duration*target))
		}
		Text {
			id: timetext
			property int secs: 0
			anchors.top: parent.top
			anchors.bottom: parent.bottom
			anchors.right: volumeslider.left
			width: contentWidth
			text: "%1/%2".arg(Util.msecToString(secs*1000.0)).arg(Util.msecToString(player.duration))
			font.family: Util.monospace
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
			value: player.volume*1e-2
			onPressed: player.setVolume(Math.floor(100.0*target))
			onDragged: player.setVolume(Math.floor(100.0*target))
		}
		property bool __hidden: false
		function show() {
			controls.anchors.bottomMargin = 0
			controls.__hidden = false
		}
		function hide() {
			if (!controls.__hidden) {
				hider.start()
				controls.__hidden = true
			}
		}
	}

	MouseArea {
		anchors.fill: parent
		hoverEnabled: player.fullScreen
		onPressed: {mouse.accepted = false}
		onPositionChanged: {
			if (player.fullScreen) {
				var my = parent.height - mouse.y;
				var contains = 0 <= my && my <= controls.height;
				if (contains)
					controls.show()
				else
					controls.hide()
			} else
				controls.show()
			mouse.accepted = false
		}
	}
	NumberAnimation {
		id: hider; target: controls; property: "anchors.bottomMargin"
		duration: 200; from: 0; to: -controls.height
	}
}

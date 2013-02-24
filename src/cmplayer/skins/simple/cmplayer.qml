import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core
Rectangle {
	id: main
	Skin.Player {
		id: player
		width: parent.width; height: fullScreen ? parent.height : parent.height - controls.height
		onFullScreenChanged: controls.update()
	}
	Skin.MouseCatcher {
		id: catcher
		width: parent.width; height: controls.height; anchors.bottom: parent.bottom
		tracking: player.fullScreen
		onCatchedChanged: controls.update()
		Rectangle {
			id: controls;
			width: parent.width; height: 20
			gradient: Gradient {
				GradientStop { position: 0.0; color: "#aaa" }
				GradientStop { position: 0.1; color: "#eee" }
				GradientStop { position: 1.0; color: "#aaa" }
			}
			Skin.HorizontalLayout {
				anchors.fill: parent; spacing: 3; paddings: 4
				fillers: [timeslider]
				Skin.Button {
					id: playPause
					width: parent.contentHeight; height: parent.contentHeight
					icon: (player.state === Core.Engine.Playing) ? "pause.png" : "play.png"
					action: "menu/play/pause"
					paddings: pressed ? 2 : (hovered ? 0 : 1)
				}
				Slider {
					id: timeslider
					value: player.time/player.duration
					onPressed: player.seek(Math.floor(player.duration*target))
				}
				Row {
					width: childrenRect.width
					Skin.TimeText { color: "black"; msecs: player.time }
					Skin.TimeText { color: "black"; text: "/" }
					Skin.TimeText { color: "black"; msecs: player.duration }
				}
				Slider {
					id: volumeslider
					width: 100
					value: player.volume*1e-2
					onPressed: player.setVolume(Math.floor(100.0*target))
					onDragged: player.setVolume(Math.floor(100.0*target))
				}
			}
			function update() {
				if (player.fullScreen && !catcher.catched)
					sliding.start()
				else
					controls.y = 0
			}
			NumberAnimation {
				id: sliding; target: controls; property: "y"
				duration: 200; from: 0; to: controls.height
			}
		}
	}
}

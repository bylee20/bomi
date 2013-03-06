import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.AppWithDock {
	id: app

	Component {
		id: slider
		Skin.Slider {
			groove: Rectangle {
				height: parent.height; anchors.verticalCenter: parent.verticalCenter;
				color: "#fff"; border { color: "#999"; width: 1 }
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#555"}
					GradientStop {position: 1.0; color: "#bbb"}
				}
			}
			filled: Rectangle {
				height: parent.height; anchors {top: parent.top; bottom: parent.bottom; left: parent.left; margins: 1}
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#fff"}
					GradientStop {position: 1.0; color: "#ccc"}
				}
			}
		}
	}

	controls: Rectangle {
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
				icon: (engine.state === Core.Engine.Playing) ? "pause.png" : "play.png"
				action: "menu/play/pause"
				paddings: pressed ? 2 : (hovered ? 0 : 1)
			}
			Skin.SeekControl { id: timeslider; engine: app.engine; component: slider }
			Row {
				width: childrenRect.width
				Skin.TimeText { color: "black"; msecs: app.engine.time }
				Skin.TimeText { color: "black"; text: "/" }
				Skin.TimeText { color: "black"; msecs: app.engine.duration }
			}
			Skin.VolumeControl { id: volumeslider; width: 100; engine: app.engine; component: slider }
		}
	}
}

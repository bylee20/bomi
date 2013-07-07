import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
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
		RowLayout {
			anchors.fill: parent; spacing: 3;
			anchors.margins: 4
			Skin.Button {
				id: playPause; width: height; height: parent.height
				action: "play/pause"; icon: (engine.state === Core.Engine.Playing) ? "pause.png" : "play.png"
				paddings: pressed ? 2 : (hovered ? 0 : 1)
			}
			Skin.SeekControl { id: timeslider; engine: app.engine; component: slider; Layout.fillWidth: true; Layout.fillHeight: true }
			Row {
				width: childrenRect.width
				height: parent.height
				Skin.TimeText { color: "black"; msecs: app.engine.time }
				Skin.TimeText { color: "black"; text: "/" }
				Skin.TimeText { color: "black"; msecs: app.engine.duration }
			}
			Skin.VolumeControl { id: volumeslider; width: 100; engine: app.engine; component: slider; height: parent.height }
		}
	}
}

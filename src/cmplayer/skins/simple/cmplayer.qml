import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.AppWithDock {
	id: app

	Component {
		id: sliders
		SliderStyle {
			readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
			groove: Item {
				implicitHeight: 12;
				implicitWidth: 100;
				Rectangle {
					anchors.fill: parent
					color: "#fff"; border { color: "#999"; width: 1 }
					gradient: Gradient {
						GradientStop {position: 0.0; color: "#555"}
						GradientStop {position: 1.0; color: "#bbb"}
					}
				}
				Rectangle {
					border { color: "#999"; width: 1 }
					anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
					width: parent.width*ratio
					gradient: Gradient {
						GradientStop {position: 0.0; color: "#fff"}
						GradientStop {position: 1.0; color: "#ccc"}
					}
				}
			}
			handle: Item {}
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
				action: "play/pause"; icon: (player.state === Core.Engine.Playing) ? "pause.png" : "play.png"
				paddings: pressed ? 2 : (hovered ? 0 : 1)
			}
			Skin.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }
			Row {
				width: childrenRect.width
				height: parent.height
				Skin.TimeText { color: "black"; msecs: engine.time }
				Skin.TimeText { color: "black"; text: "/" }
				Skin.TimeText { color: "black"; msecs: engine.end }
			}
			Skin.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
		}
	}
}

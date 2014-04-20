// written by varlesh <varlesh@gmail.com>

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayer 1.0 as Cp

Cp.AppWithDock {
	id: app

	readonly property QtObject engine: Cp.App.engine
	Component {
		id: sliders
		SliderStyle {
			readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
			groove: Item {
				implicitHeight: 4;
				implicitWidth: 100;
				Rectangle {
					anchors.fill: parent
					color: "#fff"; border { color: "#999"; width: 1 }
					gradient: Gradient {
						GradientStop {position: 0.0; color: "#acacac"}
						GradientStop {position: 1.0; color: "#acacac"}
					}
				}
				Rectangle {
					border { color: "#999"; width: 1 }
					anchors {top: parent.top; bottom: parent.bottom; left: parent.left; }
					width: parent.width*ratio
					gradient: Gradient {
						GradientStop {position: 0.0; color: "#6699cc"}
						GradientStop {position: 1.0; color: "#6699cc"}
					}
				}
			}
			handle: Image {
				width: 16; height: 16
				source: control.pressed ? "handle-pressed.png" : control.hovered ? "handle-hovered.png" : "handle.png"
				anchors.verticalCenter: parent.verticalCenter
			}
		}
	}

	controls: Rectangle {
		width: parent.width; height: 32
		gradient: Gradient {
			GradientStop { position: 0.0; color: "#aaa" }
			GradientStop { position: 0.1; color: "#eee" }
			GradientStop { position: 1.0; color: "#aaa" }
		}
		RowLayout {
			anchors { fill: parent; margins: 4 } spacing: 3;

			Cp.Button {
				id: playPrev; width: 24; height: 24
				action: "play/prev"; icon: getStateIconName("previous", hovered, pressed)
			}

			Cp.Button {
				id: playPause; width: 24; height: 24
				action: "play/pause"; icon: getStateIconName(engine.running ? "pause" : "play", hovered, pressed)
			}

			Cp.Button {
				id: playNext; width: 24; height: 24
				action: "play/next"; icon: getStateIconName("next", hovered, pressed)
			}

			Cp.TimeSlider { id: timeslider; style: sliders; Layout.fillWidth: true; Layout.fillHeight: true }

			Row {
				width: childrenRect.width; height: parent.height
				Cp.TimeText { textColor: "#1e1e1e"; msecs: engine.time; height: 12 }
				Cp.TimeText { textColor: "#1e1e1e"; text: "/"; height: 12 }
				Cp.TimeText { textColor: "#1e1e1e"; msecs: engine.end; height: 12 }
			}

			Row {
				Cp.Button {
				id: playlistIcon; width: 24; height: 24
				action: "tool/playlist/toggle"; icon: getStateIconName("playlist", hovered, pressed); action2: "tool/playlist"
							tooltip: makeToolTip(qsTr("Show/Hide Playlist"), qsTr("Show Playlist Menu"))
					}
				Cp.Button {
				id: fullscreen; width: 24; height: 24
				action: "window/full"; icon: getStateIconName("fullscreen", hovered, pressed)
					}
				Cp.Button {
				id: mute; width: 24; height: 24
				action: "audio/volume/mute"; icon: engine.muted ? getStateIconName("speaker-off", hovered, pressed) : getStateIconName("speaker-on", hovered, pressed)
					}
			}
			Cp.VolumeSlider { id: volumeslider; width: 100; style: sliders; height: parent.height }
		}
	}
}

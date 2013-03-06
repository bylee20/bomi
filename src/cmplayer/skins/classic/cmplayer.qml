import QtQuick 2.0
import CMPlayerCore 1.0 as Core
import CMPlayerSkin 1.0 as Skin

Skin.AppWithDock {
	id: app
	readonly property real margin: 5
	Component {
		id: slider
		Skin.Slider {
			id: slideritem
			groove: Rectangle {
				height: 5; anchors.centerIn: parent; border { color: "#999"; width: 1 }
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#333"}
					GradientStop {position: 1.0; color: "#bbb"}
				}
			}
			filled: Rectangle {
				height: parent.height
				border { color: "#6ad"; width: 1 }
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#fff"}
					GradientStop {position: 1.0; color: "#ccc"}
				}
			}

			handle: Rectangle {
				id: handle
				width: 9; height: 9; radius: 2
				anchors.verticalCenter: parent.verticalCenter
				border.width: slideritem.pressed ? 2 : 1
				border.color: slideritem.pressed || slideritem.hovered ? "#6ad" : "#5c5c5c"

				property Gradient __dark: Gradient {
					GradientStop { position: 0.0; color: "#aaa"}
					GradientStop { position: 1.0; color: "#999"}
				}
				property Gradient __bright: Gradient {
					GradientStop { position: 0.0; color: "#fff"}
					GradientStop { position: 1.0; color: "#ccc"}
				}
				gradient: slideritem.pressed || slideritem.hovered ? __bright : __dark
			}
		}
	}

	controls: Column {
		id: controls; width: parent.width;
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
			Skin.HorizontalLayout {
				anchors {fill: parent} spacing: 1; paddings: 2; bottomPadding: 1; fillers: [right]
				FramedButton { id: pause; width: height; action: "menu/play/pause"; icon: app.engine.playing ? "pause.png" : "play.png" }
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
						Skin.HorizontalLayout {
							anchors.fill: parent; fillers: [medianame]; paddings: 3
							Text {
								id: medianumber
								width: contentWidth; verticalAlignment: Text.AlignVCenter
								text: "[%1/%2](%3) ".arg(playlist.loaded+1).arg(playlist.count).arg(app.engine.stateText)
								font { pixelSize: 11; family: Core.Util.monospace }
							}
							Text {
								id: medianame
								text: app.engine.media.name; elide: Text.ElideMiddle
								font { pixelSize: 11; family: Core.Util.monospace }
								verticalAlignment: Text.AlignVCenter
							}
							Row {
								width: childrenRect.width
								Skin.TimeText { color: "black"; font.pixelSize: 11; msecs: app.engine.time }
								Skin.TimeText { color: "black"; font.pixelSize: 11; text: "/" }
								Skin.TimeText { color: "black"; font.pixelSize: 11; msecs: app.engine.duration }
							}
						}
					}
					Skin.HorizontalLayout {
						width: parent.width; height: pause.height-1-panel.height; fillers: [timeslider]; spacing: 1
						Skin.SeekControl { id: timeslider; engine: app.engine; component: slider }
						FramedButton {
							id: mute; width: height; action: "menu/audio/mute"
							icon: app.engine.muted ? "speaker-off.png" : "speaker-on.png"
						}
						Skin.VolumeControl { id: volumeslider; width: 70; engine: app.engine; component: slider }
					}
				}
			}
		}
	}
}

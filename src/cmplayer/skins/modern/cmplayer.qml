import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.AppWithFloating {
	id: app
	name: "net.xylosper.cmplayer.skin.modern"
	engine: Skin.Player { anchors.fill: parent }

	Component {
		id: slider
		Skin.Slider {
			id: slideritem
			height: 5
			groove: Rectangle {
				height: 5; radius: 2; border { color: "#ccc"; width: 1 }
				anchors.verticalCenter: parent.verticalCenter
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#333"}
					GradientStop {position: 1.0; color: "#bbb"}
				}
			}
			filled: Rectangle {
				height: groove.height; radius: groove.radius; border {width: 1; color: "#5af"}
				gradient: Gradient {
					GradientStop { position: 0.0; color: "white" }
					GradientStop { position: 1.0; color: "skyblue" }
				}
			}
			handle: Image {
				width: 10; height: 10
				source: slideritem.pressed ? "handle-pressed.png" : slideritem.hovered ? "handle-hovered.png" : "handle.png"
				anchors.verticalCenter: parent.verticalCenter
			}
		}
	}

	controls: Item {
		width: 400; height: inner.height+24
		BorderImage {
			id: bg; source: "bg.png"; anchors.fill: parent; opacity: 0.9
			border {left: 15; right: 15; top: 15; bottom: 35}
		}
		Column {
			id: inner; width: parent.width-50; anchors.centerIn: parent; spacing: 5
			Skin.HorizontalLayout {
				id: texts; width: parent.width; height: 15; fillers: [name]; spacing: 5
				property int vAlignment: Text.AlignBottom
				Skin.TimeText { id: position; msecs: engine.time; verticalAlignment: texts.vAlignment }
				Text {
					id: name
					text: engine.media.name; elide: Text.ElideMiddle;
					color: "white"; font { bold: true; pixelSize: 12 }
					horizontalAlignment: Text.AlignHCenter; verticalAlignment: texts.vAlignment
				}
				Skin.TimeText { id: duration; msecs: engine.duration; verticalAlignment: texts.vAlignment}
			}
			Skin.HorizontalLayout {
				id: seekbarwrapper; width: parent.width; height: 10; fillers: [seekbar]; spacing: 10
				Skin.SeekControl { id: seekbar; component: slider; engine: app.engine }
			}
			Item {
				id: buttons; width: parent.width; height: 22
				Skin.HorizontalLayout {
					height: parent.height*0.75; anchors.verticalCenter: parent.verticalCenter; spacing: 3
					Skin.Button {
						id: mute; checked: engine.muted; width: parent.height; height: parent.height
						icon: getStateIconName("volume"); action: "audio/mute"
						Item {
							id: volume; anchors.fill: parent
							visible: (!mute.checked && !(mute.hovered && mute.pressed))
									 || (mute.checked && mute.pressed && mute.hovered)
							Image {
								id: volume1; anchors.fill: parent; visible: engine.volume > 10
								source: mute.hovered ? "volume-1-hovered.png" : "volume-1.png"
							}
							Image {
								id: volume2; anchors.fill: parent; visible: engine.volume > 40
								source: mute.hovered ? "volume-2-hovered.png" : "volume-2.png"
							}
							Image {
								id: volume3; anchors.fill: parent; visible: engine.volume > 80
								source: mute.hovered ? "volume-3-hovered.png" : "volume-3.png"
							}
						}
					}
					Skin.VolumeControl { id: volumebar; width: 70; component: slider; engine: app.engine }
				}
				Row {
					height: parent.height; spacing: 10; anchors.horizontalCenter: parent.horizontalCenter;
					Skin.Button {
						width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
						icon: getStateIconName("play-slower"); action: "play/speed/slower"
					}
					Skin.Button {
						id: pause; width: parent.height; height: width
						icon: getStateIconName(engine.playing ? "pause" : "play"); action: "play/pause"
					}

					Skin.Button {
						id: faster; width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
						icon: getStateIconName("play-faster"); action: "play/speed/faster"
					}
				}
			}
		}
		Skin.Button {
			id: toggler; parent: checked ? seekbarwrapper : buttons; icon: getStateIconName("toggle")
			width: 20; height: 10; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
			onClicked: checked = !checked
			states: State {
				name: "toggled"; when: toggler.checked
				PropertyChanges { target:texts; opacity: 0.0; vAlignment: Text.AlignTop; height: 0.0 }
				PropertyChanges { target:buttons; opacity: 0.0; height: 0.0 }
				PropertyChanges { target:inner; spacing: 0 }
				PropertyChanges { target:bg; border.bottom: 15 } // to adjust bg center line
			}
			transitions: Transition {
				reversible: true; to: "toggled"
				SequentialAnimation {
					NumberAnimation { property: "opacity"; duration: 150 }
					NumberAnimation { property: "spacing"; duration: 50 }
					ParallelAnimation {
						NumberAnimation { property: "height"; duration: 150 }
						NumberAnimation { property: "border.bottom"; duration: 150 }
					}
				}
			}
		}
	}
	Component.onCompleted: {
		Core.Settings.open(app.name)
		toggler.checked = Core.Settings.getBool("toggled", false)
		Core.Settings.close()
	}
	Component.onDestruction: {
		Core.Settings.open(app.name)
		Core.Settings.set("toggled", toggler.checked)
		Core.Settings.close()
	}
}

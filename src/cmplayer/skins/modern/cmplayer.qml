import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import CMPlayer 1.0 as Cp

Cp.AppWithFloating {
	id: app
	name: "net.xylosper.cmplayer.skin.modern"
	Component {
		id: slider
		SliderStyle {
			readonly property real ratio: (control.value - control.minimumValue)/(control.maximumValue - control.minimumValue)
			groove: Rectangle {
				height: 5; radius: 2; border { color: "#ccc"; width: 1 }
				anchors.verticalCenter: parent.verticalCenter
				gradient: Gradient {
					GradientStop {position: 0.0; color: "#333"}
					GradientStop {position: 1.0; color: "#bbb"}
				}
				Rectangle {
					width: parent.width*ratio; height: parent.height
					radius: parent.radius; border {width: 1; color: "#5af"}
					gradient: Gradient {
						GradientStop { position: 0.0; color: "white" }
						GradientStop { position: 1.0; color: "skyblue" }
					}
				}
			}
			handle: Image {
				width: 10; height: 10
				source: control.pressed ? "handle-pressed.png" : control.hovered ? "handle-hovered.png" : "handle.png"
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
			RowLayout {
				id: texts; width: parent.width; height: 15; spacing: 5
				Cp.TimeText { id: position; msecs: engine.time; Layout.alignment: Qt.AlignBottom; textAlignmentV: Text.AlignBottom }
				Text {
					id: name
					Layout.alignment: Qt.AlignBottom; Layout.fillWidth: true
                    text: engine.media.name; elide: Text.ElideMiddle;
					color: "white"; font { bold: true; pixelSize: 12 }
					horizontalAlignment: Text.AlignHCenter
				}
				Cp.TimeText { id: duration; msecs: engine.end; Layout.alignment: Qt.AlignBottom; textAlignmentV: Text.AlignBottom }
			}
			RowLayout {
				id: seekbarwrapper; width: parent.width; height: 10; spacing: 10
				Cp.TimeSlider { id: seekbar; style: slider; Layout.fillWidth: true }
			}
			Item {
				id: buttons; width: parent.width; height: 22
				RowLayout {
					height: parent.height*0.75; anchors.verticalCenter: parent.verticalCenter; spacing: 3
					Cp.Button {
                        id: mute; checked: engine.muted; width: parent.height; height: parent.height
						icon: getStateIconName("volume"); action: "audio/volume/mute"
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
					Cp.VolumeSlider { id: volumebar; width: 70; style: slider }
				}
				Row {
					height: parent.height; spacing: 10; anchors.horizontalCenter: parent.horizontalCenter;
					Cp.Button {
						width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
                        icon: getStateIconName("seek-backward"); action: "play/seek/backward2"
					}
					Cp.Button {
						id: pause; width: parent.height; height: width
                        icon: getStateIconName(engine.running ? "pause" : "play"); action: "play/pause"
					}

					Cp.Button {
						id: faster; width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
						icon: getStateIconName("seek-forward"); action: "play/seek/forward2"
					}
				}
			}
		}
		Cp.Button {
			id: toggler; parent: checked ? seekbarwrapper : buttons; icon: getStateIconName("toggle")
			width: 20; height: 10; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
			onClicked: checked = !checked
			states: State {
				name: "toggled"; when: toggler.checked
				PropertyChanges { target:texts; opacity: 0.0; height: 0.0 }
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
		Cp.Settings.open(app.name)
		toggler.checked = Cp.Settings.getBool("toggled", false)
		Cp.Settings.close()
	}
	Component.onDestruction: {
		Cp.Settings.open(app.name)
		Cp.Settings.set("toggled", toggler.checked)
		Cp.Settings.close()
	}
}

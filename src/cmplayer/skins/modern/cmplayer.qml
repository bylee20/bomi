import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.AppWithFloating {
	id: app
	name: "net.xylosper.cmplayer.skin.modern"
	player: Skin.Player { anchors.fill: parent }
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
				Skin.TimeText { id: position; msecs: app.player.time; verticalAlignment: texts.vAlignment }
				Text {
					id: name
					text: app.player.media.name; elide: Text.ElideMiddle;
					color: "white"; font { bold: true; pixelSize: 12 }
					horizontalAlignment: Text.AlignHCenter; verticalAlignment: texts.vAlignment
				}
				Skin.TimeText { id: duration; msecs: app.player.duration; verticalAlignment: texts.vAlignment}
			}
			Skin.HorizontalLayout {
				id: seekbarwrapper; width: parent.width; height: 10; fillers: [seekbar]; spacing: 10
				Slider { id: seekbar; value: app.player.time/app.player.duration; onDragging: app.player.seek(app.player.duration*value) }
			}
			Item {
				id: buttons; width: parent.width; height: 22
				Row {
					height: parent.height*0.75; anchors.verticalCenter: parent.verticalCenter; spacing: 3
					Skin.Button {
						id: mute; checked: app.player.muted; width: parent.height; height: parent.height
						icon: getStateIconName("volume"); action: "menu/audio/mute"
						Item {
							id: volume; anchors.fill: parent
							visible: (!mute.checked && !(mute.hovered && mute.pressed))
									 || (mute.checked && mute.pressed && mute.hovered)
							Image {
								id: volume1; anchors.fill: parent; visible: app.player.volume > 10
								source: mute.hovered ? "volume-1-hovered.png" : "volume-1.png"
							}
							Image {
								id: volume2; anchors.fill: parent; visible: app.player.volume > 40
								source: mute.hovered ? "volume-2-hovered.png" : "volume-2.png"
							}
							Image {
								id: volume3; anchors.fill: parent; visible: app.player.volume > 80
								source: mute.hovered ? "volume-3-hovered.png" : "volume-3.png"
							}
						}
					}
					Slider {
						id: volumebar; anchors.verticalCenter: parent.verticalCenter; width: 70
						value: app.player.volume/100; onDragging: app.player.volume = value*100
					}
				}
				Row {
					height: parent.height; spacing: 10; anchors.horizontalCenter: parent.horizontalCenter;
					Skin.Button {
						width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
						icon: getStateIconName("play-slower"); action: "menu/play/speed/slower"
					}
					Skin.Button {
						id: pause; width: parent.height; height: width
						icon: getStateIconName(app.player.playing ? "pause" : "play"); action: "menu/play/pause"
					}

					Skin.Button {
						id: faster; width: parent.height*0.9; height: width; anchors.verticalCenter: pause.verticalCenter
						icon: getStateIconName("play-faster"); action: "menu/play/speed/faster"
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

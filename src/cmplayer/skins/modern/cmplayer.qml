import QtQuick 2.0
import CMPlayerSkin 1.0 as Skin
import CMPlayerCore 1.0 as Core

Skin.Player {
	id: player
	dockZ: 1.0
	property real pw: -1
	onWidthChanged: {
		if (pw >= 0)
			controls.setCx(controls.getCx(pw))
		pw = width
	}
	property real ph: -1
	onHeightChanged:  {
		if (ph >= 0)
			controls.setCy(controls.getCy(ph))
		ph = height
	}

	MouseArea {
		Component.onCompleted: controls.hidden = !containsMouse
		anchors.fill: parent
		hoverEnabled: true; onPressed: mouse.accepted = false
		onEntered: controls.hidden = false; onExited: controls.hidden = true
		MouseArea {
			id: controls //Math.max(0, Math.min(cx*parent.width - width/2, parent.width-width)) }//
			function setCx(cx) { x = Core.Util.bound(0, cx*parent.width - width/2, parent.width - width) }
			function setCy(cy) { y = Core.Util.bound(0, cy*parent.height - height/2, parent.height - height) }
			function getCx(bg) { return (x+width/2)/bg; }
			function getCy(bg) { return (y+height/2)/bg; }

//			property real cx: -1//getCx()
//			property real cy: -1//getCy()
//			onXChanged: cx = getCx()
//			onWidthChanged: cx = getCx()
//			Component.onCompleted: {cx = getCx()}
			property bool hidden: false
			width: 400; height: inner.height+24
			drag.target: controls; drag.axis: Drag.XAndYAxis
			drag.minimumX: 0; drag.maximumX: player.width-width
			drag.minimumY: 0; drag.maximumY: player.height-height
			onDoubleClicked: Util.filterDoubleClick()
			BorderImage {
				id: bg; source: "bg.png"; anchors.fill: parent; opacity: 0.9
				border {left: 15; right: 15; top: 15; bottom: 35}
			}
			Column {
				id: inner; width: parent.width-50; anchors.centerIn: parent; spacing: 5
				Skin.HorizontalLayout {
					id: texts; width: parent.width; height: 15; fillers: [name]; spacing: 5
					property int vAlignment: Text.AlignBottom
					TimeText { id: position; msecs: player.time; verticalAlignment: texts.vAlignment }
					Text {
						id: name
						text: player.media.name; elide: Text.ElideMiddle;
						color: "white"; font { bold: true; pixelSize: 12 }
						horizontalAlignment: Text.AlignHCenter; verticalAlignment: texts.vAlignment
					}
					TimeText { id: duration; msecs: player.duration; verticalAlignment: texts.vAlignment}
				}
				Skin.HorizontalLayout {
					id: seekbarwrapper; width: parent.width; height: 10; fillers: [seekbar]; spacing: 10
					Slider { id: seekbar; value: player.time/player.duration; onDragging: player.seek(player.duration*value) }
				}
				Item {
					id: buttons; width: parent.width; height: 22
					Row {
						height: parent.height*0.75; anchors.verticalCenter: parent.verticalCenter; spacing: 3
						Skin.Button {
							id: mute; checked: player.muted; width: parent.height; height: parent.height
							icon: getStateIconName("volume"); action: "menu/audio/mute"
							Item {
								id: volume; anchors.fill: parent
								visible: (!mute.checked && !(mute.hovered && mute.pressed))
									   || (mute.checked && mute.pressed && mute.hovered)
								Image {
									id: volume1; anchors.fill: parent; visible: player.volume > 10
									source: mute.hovered ? "volume-1-hovered.png" : "volume-1.png"
								}
								Image {
									id: volume2; anchors.fill: parent; visible: player.volume > 40
									source: mute.hovered ? "volume-2-hovered.png" : "volume-2.png"
								}
								Image {
									id: volume3; anchors.fill: parent; visible: player.volume > 80
									source: mute.hovered ? "volume-3-hovered.png" : "volume-3.png"
								}
							}
						}
						Slider {
							id: volumebar; anchors.verticalCenter: parent.verticalCenter; width: 70
							value: player.volume/100; onDragging: player.volume = value*100
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
							icon: getStateIconName(player.playing ? "pause" : "play"); action: "menu/play/pause"
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
			states: State {
				name: "hidden"; when: controls.hidden
				PropertyChanges { target: controls; opacity: 0.0 }
				PropertyChanges { target: controls; visible: false }
			}
			transitions: Transition {
				reversible: true; to: "hidden"
				SequentialAnimation {
					NumberAnimation { property: "opacity"; duration: 200 }
					PropertyAction { property: "visible" }
				}
			}
		}
		Connections { target: Core.Util; onCursorVisibleChanged: controls.hidden = !cursorVisible }
	}
	readonly property string name: "net.xylosper.cmplayer.skin.modern"
	Component.onCompleted: {
		Core.Settings.open(player.name)
		toggler.checked = Core.Settings.get("toggled")
		Core.Settings.close()
	}
	Component.onDestruction: {
		Core.Settings.open(player.name)
		Core.Settings.set("toggled", toggler.checked)
		Core.Settings.close()
	}
}

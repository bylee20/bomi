import QtQuick 2.0
import CMPlayerCore 1.0 as Core
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Item {
	id: player
	objectName: "player"
	property real dockZ: 0.0
	property real bottomPadding: 0.0
	Logo { anchors.fill: parent }
	TextOsd { id: msgosd }
	Rectangle {
		id: msgbox
		color: "#ddd"
		opacity: 0.8
		visible: false
		anchors.centerIn: parent
		width: boxmsg.width + pad*2
		height: boxmsg.height + pad*2
		readonly property real pad: 5
		Text {
			id: boxmsg
			anchors.centerIn: parent
			opacity: 1.0
			width: contentWidth
			height: contentHeight
			font.bold: true
		}
		radius: 5
		border.color: "black"
		border.width: 1
	}

	ProgressOsd { id: timeline; value: engine.relativePosition }
	PlayInfoView { objectName: "playinfo" }
	Item {
		anchors.fill: parent; z: dockZ
		PlaylistDock {
			id: right
			objectName: "playlist"
			width: Math.min(widthHint, player.width-(left.x+left.width)-20)
			height: parent.height-2*y - bottomPadding
		}
		HistoryDock {
			id: left
			objectName: "history"
			width: Math.min(widthHint, player.width*0.4)
			height: parent.height-2*y - bottomPadding
		}
	}
	Component.onCompleted: {
		engine.screen.parent = player
		engine.screen.width = width
		engine.screen.height = height
	}
	onWidthChanged: engine.screen.width = width
	onHeightChanged: engine.screen.height = height

	function showOSD(msg) { msgosd.text = msg; msgosd.show(); }
	function showMessageBox(msg) { msgbox.visible = !!msg; boxmsg.text = msg }

	Connections {
		target: engine;
		onSought: {timeline.show();}
	}
}

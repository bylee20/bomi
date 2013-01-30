import QtQuick 2.0
import CMPlayerSkin 1.0

Engine {
	id: player
	Logo { anchors.fill: parent; visible: player.state == Engine.Stopped }
	TextOsd { id: msgosd }
	ProgressOsd { id: timeline }
	PlayInfoOsd { objectName: "playinfo"; player: player }
	PlaylistDock { objectName: "playlist" }
	HistoryDock { objectName: "history" }

	onMessageRequested: { msgosd.text = message; msgosd.show(); }
	onTick: { timeline.value = time/duration }
	onSought: {timeline.show();}
	function showSize() { msgosd.text = "%1x%2".arg(width).arg(height); msgosd.show() }
	onHeightChanged: showSize()
	onWidthChanged: showSize()
}

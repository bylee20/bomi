import QtQuick 2.0
import CMPlayer 1.0
import "qml"

Rectangle {
	Player {
		id: player
		property string name: "player"
		anchors.fill: parent
		infoView: PlayInfoOsd {
			parent: player
			info: player.info

		}
		TextOsd {
			id: msgosd
			anchors.fill: parent
		}
		ProgressOsd {
			id: timeline
			anchors.fill: parent
		}
		info.onTick: timeline.value = info.time/info.duration
		onMessageRequested: {msgosd.text = message; msgosd.show();}
		onSought: {timeline.show();}
	}
	Rectangle {
		id: controls

	}
}

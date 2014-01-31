import QtQuick 2.0
import CMPlayer 1.0

Text {
	id: item
	font { pixelSize: parent.fontSize; family: Util.monospace }
	color: "yellow"; style: Text.Outline; styleColor: "black"

	property alias timer: t.running
	property alias interval: t.interval
	signal tick

	Timer { id: t; interval: 1000; repeat: true; onTriggered: item.tick() }
}

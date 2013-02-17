import QtQuick 2.0

Osd {
	id: osd
	property alias text: text.text
	Text {
		id: text
		color: "white"
		style: Text.Outline
		styleColor: "black"
		anchors.fill: parent
		onWidthChanged: {font.pixelSize = width*0.025;}
	}
}

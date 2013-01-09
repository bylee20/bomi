import QtQuick 2.0

Rectangle {
	id: logo
	Image {
		anchors.fill: parent
		source: "bg.svg"
		sourceSize.height: parent.height
		sourceSize.width: parent.width
	}
	Image {
		id: logoImage
		anchors.centerIn: parent
		source: "cmplayer.png"
		smooth: true
		width: Math.min(logo.width*0.7, logo.height*0.7, 512)
		height:width
	}
}

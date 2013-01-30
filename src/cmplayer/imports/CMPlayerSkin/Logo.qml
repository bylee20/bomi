import QtQuick 2.0

Rectangle {
	id: logo
	Image {
		anchors.fill: parent
		source: "qrc:/img/logo-background.png"
        smooth: true
	}
	Image {
		id: logoImage
		anchors.centerIn: parent
		source: "qrc:/img/cmplayer-logo.png"
		smooth: true
		width: Math.min(logo.width*0.7, logo.height*0.7, 512)
		height:width
	}
}

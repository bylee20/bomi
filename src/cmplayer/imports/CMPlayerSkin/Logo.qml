import QtQuick 2.0
import CMPlayerCore 1.0

Rectangle {
	id: logo
	objectName: "logo"
	property bool show: false
	color: "black"
	visible: engine.state == Engine.Stopped || !engine.hasVideo
	Image {
        visible: logo.show
		anchors.fill: parent
		source: "qrc:/img/logo-background.png"
        smooth: true
		Image {
			id: logoImage
			anchors.centerIn: parent
			source: "qrc:/img/cmplayer-logo.png"
			smooth: true
			width: Math.min(logo.width*0.7, logo.height*0.7, 512)
			height:width
		}
	}
}

import QtQuick 2.0
import bomi 1.0

Rectangle {
    id: logo
    objectName: "logo"
    property bool show: false
    color: "black"
    visible: true
    Image {
        visible: logo.show
        anchors.fill: parent
        source: "qrc:/img/logo-background.svg"
        smooth: true
        Image {
            id: logoImage
            anchors.centerIn: parent
            source: "qrc:/img/logo-center.png"
            smooth: true
            width: Math.min(logo.width*0.5, logo.height*0.5, 256)
            height:width
        }
    }
    Timer {
        id: timer
        repeat: false
        interval: 500
        property bool visible: App.engine.stopped || !App.engine.hasVideo
        onVisibleChanged: {
            if (visible) {
                timer.start()
            } else {
                timer.stop()
                logo.visible = false
            }
        }
        onTriggered: { logo.visible = timer.visible }
    }
    Component.onCompleted: {
        if (!timer.visible)
            visible = false
    }
}

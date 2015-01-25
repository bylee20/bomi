import QtQuick 2.0
import QtQuick.Controls 1.0 as Qc
import bomi 1.0

Item {
    id: seeker
    property alias min: slider.minimumValue
    property alias max: slider.maximumValue
    property alias value: slider.value
    property alias style: slider.style
    property real range: slider.range
    property alias orientation: slider.orientation
    property alias rate: slider.rate
    MouseArea {
        anchors.fill: parent
        Qc.Slider {
            id: slider
            anchors.fill: parent
            property alias min: slider.minimumValue
            property alias max: slider.maximumValue
            property real range: max - min
            property real rate: (value - min)/(max - min)
        }
        onWheel: wheel.accepted = true
        onPressed: mouse.accepted = false
    }
}

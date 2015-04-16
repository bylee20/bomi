import QtQuick 2.0
import QtQuick.Controls 1.0 as Qc
import bomi 1.0

Item {
    id: seeker
    property alias min: slider.minimumValue
    property alias max: slider.maximumValue
    property real ahead: 0
    property alias value: slider.value
    property alias style: slider.style
    property alias orientation: slider.orientation
    readonly property real range: slider.range
    readonly property alias rate: slider.rate
    readonly property alias arate: slider.arate
    readonly property alias hpressed: slider.hpressed
    readonly property alias hhovered: slider.hhovered

    property alias __hpressed: slider.hpressed
    property alias __hhovered: slider.hhovered

    implicitHeight: slider.implicitHeight
    MouseArea {
        anchors.fill: parent
        Qc.Slider {
            id: slider
            anchors.fill: parent
            readonly property alias min: slider.minimumValue
            readonly property alias max: slider.maximumValue
            readonly property alias ahead: seeker.ahead
            readonly property real range: max - min
            readonly property real rate: (value - min)/(max - min)
            readonly property real arate: ahead == 0 ? 0 : ahead < 0 ? 1.0 : (ahead + value - min)/(max - min)
            property bool hpressed: pressed
            property bool hhovered: hovered
        }
        onWheel: wheel.accepted = true
        onPressed: mouse.accepted = false
    }
}

import QtQuick 2.0
import QtQuick.Controls 1.0 as Qc
import bomi 1.0

Item {
    id: seeker
    property real ahead: 0
    property alias min: slider.minimumValue
    property alias max: slider.maximumValue
    property alias value: slider.value
    property alias style: slider.style
    property alias orientation: slider.orientation
    readonly property alias range: slider.range
    readonly property alias rate: slider.rate
    readonly property alias arate: slider.arate
    readonly property alias hpressed: slider.hpressed
    readonly property alias hhovered: slider.hhovered
    property bool acceptsWheel: false

    property alias __hpressed: slider.hpressed
    property alias __hhovered: slider.hhovered

    implicitHeight: slider.implicitHeight

    MouseArea {
        anchors.fill: parent
        onWheel: wheel.accepted = true
        onPressed: mouse.accepted = false
    }
    Qc.Slider {
        id: slider
        anchors.fill: parent
        stepSize: 0
        readonly property alias min: slider.minimumValue
        readonly property alias max: slider.maximumValue
        readonly property real ahead: Math.min(seeker.ahead, max)
        readonly property real range: max - min
        readonly property real rate: (value - min)/(max - min)
        readonly property real arate: ahead == 0 ? 0 : (ahead - min)/(max - min)
        property bool hpressed: pressed
        property bool hhovered: mouseArea.containsMouse
        MouseArea {
            id: mouseArea
            hoverEnabled: true
            anchors.fill: parent
            onWheel: wheel.accepted = !acceptsWheel
            onPressed: mouse.accepted = false
        }
    }
}

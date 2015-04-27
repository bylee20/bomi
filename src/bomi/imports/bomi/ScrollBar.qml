import QtQuick 2.0
import bomi 1.0

Item { id: item;
    property Flickable target
    property alias color: handle.color
    property alias border: handle.border
    property alias radius: handle.radius
    property alias minimumLength: handle.hMin
    property real gap: 5
    readonly property alias pressed: mouseArea.pressed
    opacity: 0

    Item {
        anchors { fill: parent; margins: gap }
        Rectangle {
            id: handle
            color: Qt.rgba(0.0, 0.0, 0.0, 0.5)
            readonly property real rh: item.target.visibleArea.heightRatio
            readonly property real ry: item.target.visibleArea.yPosition
            readonly property real trgt: (ry + rh*0.5)*parent.height
            property real hMin: 35
            readonly property real half: Math.max(rh*parent.height, hMin)*0.5
            x: 0; y: Math.min(Math.max(0, trgt - half), parent.height)
            width: parent.width; height: half + size()
            function size() {
                if (trgt < half)
                    return trgt
                var dt = parent.height - trgt;
                return (dt < half) ? dt : half;
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        property bool moving: false
        function move(y) {
            var min = 0, max = 1
            if (target.headerItem)
                min -= target.headerItem.height / target.contentHeight
            if (target.footerItem)
                max -= target.footerItem.height / target.contentHeight
            var rate = Alg.clamp((y-gap)/(height-2*gap), min, max)
            target.contentY = (target.contentHeight - target.height)*rate
        }
        onPressed: move(mouse.y)
        onPositionChanged: if (pressed) move(mouse.y)
        hoverEnabled: true
        Timer {
            id: timer
            running: false
            repeat: false
            interval: 500
            onTriggered: mouseArea.moving = false
        }
        preventStealing: true
        Connections {
            target: item.target
            onContentYChanged: {
                mouseArea.moving = true
                timer.restart()
            }
            onMovingVerticallyChanged: {
                if (item.target.movingVertically) {
                    mouseArea.moving = true
                    timer.restart()
                }
            }
        }
        onWheel: target.wheelScroll(wheel.angleDelta, wheel.pixelDelta)
    }

    states: State {
        name: "visible"
        when: target && target.visibleArea.heightRatio < 1
              && (target.movingVertically || mouseArea.moving || mouseArea.containsMouse)
        PropertyChanges { target: item; opacity: 1.0 }
    }

    transitions: Transition {
        from: "visible"; to: ""
        NumberAnimation { properties: "opacity"; duration: 600 }
    }
}



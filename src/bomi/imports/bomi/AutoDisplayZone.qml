import QtQuick 2.0
import bomi 1.0 as B

Item {
    property alias target: hideTimer.target
    property Item box: zone
    property alias showDelay: showTimer.interval
    property alias hideDelay: hideTimer.timeout
    property bool auto: false
    readonly property alias hovered: zone.hovered
    readonly property int status: target.visible ? __ToolVisible
                                : hovered ? __ToolEdge : __ToolHidden

    z: 1e10;

    MouseArea {
        id: zone
        property bool hovered: containsMouse && entered
        property bool entered: false

        anchors.fill: parent
        hoverEnabled: true
        onEntered: entered = B.App.window.mouse.isIn(zone)
        onExited: entered = false
        onHoveredChanged: {
            if (hovered && status != __ToolVisible)
                B.App.window.showToolTip(zone, Qt.point(mouseX, mouseY), qsTr("Click to display"))
            else
                B.App.window.hideToolTip()
            showTimer.running = auto && hovered && target
        }
        onPressed: mouse.accepted = status != __ToolVisible
        onClicked: { if (!target.visible) display() }

        function display() { target.visible = true; hideTimer.run() }

        B.HideTimer {
            id: hideTimer; target: zone.target
            hide: function() { return !B.App.window.mouse.isIn(box) }
        }

        Timer {
            id: showTimer; repeat: false; interval: 300
            onTriggered: { if (B.App.window.mouse.isIn(zone)) zone.display() }
        }
    }
}

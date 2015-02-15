import QtQuick 2.0
import bomi 1.0 as B

Item {
    property alias target: hideTimer.target
    property Item box: zone
    property alias showDelay: showTimer.interval
    property alias hideDelay: hideTimer.timeout
    z: 1e10;
    MouseArea {
        id: zone
        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: { showTimer.running = containsMouse && target }
        B.HideTimer {
            id: hideTimer
            target: zone.target
            hide: function() { return !B.App.window.mouse.isIn(box) }
        }
        Timer {
            id: showTimer
            repeat: false
            interval: 200
            onTriggered: {
                if (B.App.window.mouse.isIn(zone)) {
                    target.visible = true
                    hideTimer.run()
                }
            }
        }
    }
}

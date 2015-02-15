import QtQuick 2.0

Item {
    id: hider
    property var target
    property alias timeout: timer.interval
    property var hide: function () { return true }
    property bool running: timer.running
    function run() {
        if (target && hide)
            timer.start()
    }

    Timer {
        id: timer
        interval: 1500
        repeat: true
        onTriggered: {
            if (hider.hide()) {
                hider.target.visible = false;
                timer.stop()
            }
        }
    }
}

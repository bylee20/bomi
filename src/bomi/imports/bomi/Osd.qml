import QtQuick 2.0

Item {
    id: osd
    visible: false
    property int duration: 2500
    Timer {
        id: timer
        interval: osd.duration
        onTriggered: {osd.visible = false;}
    }
    function show() {
        visible = true
        if (timer.running)
            timer.running = false;
        timer.running = true;
    }
}

import QtQuick 2.0
import QtQuick.Controls 1.0
import CMPlayer 1.0

Slider {
    id: seeker
    property alias min: seeker.minimumValue
    property alias max: seeker.maximumValue
    property real range: max - min
    readonly property Engine engine: App.engine
    minimumValue: engine.begin; maximumValue: engine.end
    QtObject {
        id: d;
        property bool ticking: false
    }
    Connections {
        target: engine
        onTick: {
            d.ticking = true;
            seeker.value = engine.time
            d.ticking = false;
        }
    }
    onValueChanged: if (!d.ticking) engine.seek(value)
}

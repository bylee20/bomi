import QtQuick 2.0
import bomi 1.0

Slider {
    id: seeker
    readonly property Engine engine: App.engine
    min: engine.begin; max: engine.end
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

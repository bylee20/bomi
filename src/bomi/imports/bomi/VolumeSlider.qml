import QtQuick 2.0
import bomi 1.0

Slider {
    id: item
    min: 0; max: 1
    acceptsWheel: false
    Connections { target: App.engine; onVolumeChanged: item.value = App.engine.volume }
    onValueChanged: if (value != App.engine.volume) App.engine.volume = value
    Component.onCompleted: item.value = App.engine.volume
}

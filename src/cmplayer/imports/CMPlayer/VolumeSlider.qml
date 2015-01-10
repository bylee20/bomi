import QtQuick 2.0
import CMPlayer 1.0

Slider {
    id: item
    min: 0; max: 100
    Connections { target: App.engine; onVolumeChanged: item.value = App.engine.volume }
    onValueChanged: App.engine.volume = value
    Component.onCompleted: item.value = App.engine.volume
}

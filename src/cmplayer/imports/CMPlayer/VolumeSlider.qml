import QtQuick 2.0
import QtQuick.Controls 1.0
import CMPlayer 1.0

Slider {
    id: item
    minimumValue: 0; maximumValue: 100
    Connections { target: App.engine; onVolumeChanged: item.value = App.engine.volume }
    onValueChanged: App.engine.volume = value
    Component.onCompleted: item.value = App.engine.volume
}

import QtQuick 2.0
import QtQuick.Controls 1.0
import CMPlayerCore 1.0

Slider {
	id: item
	minimumValue: 0; maximumValue: 100
	Connections { target: engine; onVolumeChanged: item.value = engine.volume }
	onValueChanged: engine.volume = value
	Component.onCompleted: item.value = engine.volume
}

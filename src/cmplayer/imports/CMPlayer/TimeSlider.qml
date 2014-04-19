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
	Connections {
		target: engine
		onTick: if (!seeker.pressed) seeker.value = engine.time
	}
	onValueChanged: if (pressed) engine.seek(value)
}

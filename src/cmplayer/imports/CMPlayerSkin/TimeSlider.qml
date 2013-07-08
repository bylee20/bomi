import QtQuick 2.0
import QtQuick.Controls 1.0
import CMPlayerCore 1.0

Slider {
	id: seeker
	property Engine engine: Engine { }
	minimumValue: 0; maximumValue: engine.duration
	Connections { target: engine; onTick: if (!seeker.pressed) seeker.value = engine.time }
	onValueChanged: if (pressed) engine.seek(value)
}

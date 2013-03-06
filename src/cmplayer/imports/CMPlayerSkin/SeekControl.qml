import QtQuick 2.0
import CMPlayerCore 1.0
import CMPlayerSkin 1.0

Item {
	property Engine engine: Engine { }
	property alias component: loader.sourceComponent
	Loader { id: loader; anchors.fill: parent }
	Binding { target: loader.item; property: "value";  value: engine.time/engine.duration }
	Connections { target: loader.item; onDragging: engine.seek(engine.duration*value) }
}

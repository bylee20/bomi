import QtQuick 2.0
import CMPlayerCore 1.0
import CMPlayerSkin 1.0

Item {
	id: item
	property Engine engine: Engine { }
	property alias component: loader.sourceComponent
	Loader { id: loader; anchors.fill: item }
	Binding { target: loader.item; property: "value"; value: engine.volume/100 }
	Connections { target: loader.item; onDragging: engine.volume = value*100 }
}

import QtQuick 2.0
import CMPlayerCore 1.0 as Core

Text {
	width: contentWidth; height: parent.height; color: "white"
	property int msecs: 0
	property int __secs: (msecs/1000.0).toFixed(0)
	text: Core.Util.msecToString(__secs*1000)
	font { pixelSize: 10; family: Core.Util.monospace }
}

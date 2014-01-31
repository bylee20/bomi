import QtQuick 2.0
import CMPlayer 1.0

TimeText {
	width: Util.textWidth(showMSecs ? "00:00:00.000" : "00:00:00", font.pixelSize, font.family);
	height: textHeight; font.pixelSize: 12;
	textColor: getStateTextColor(hovered || (bind && bind.hovered), pressed || (bind && bind.pressed))
	onClicked: checked = !checked
	textAlignmentV: Text.AlignVCenter
	action: bind ? bind.action : ""; action2: bind ? bind.action2 : ""; tooltip: bind ? bind.tooltip : ""
	function getStateTextColor(hovered, pressed) { return pressed ? "#0ef" : (hovered ? "#0cf" : "white") }
}

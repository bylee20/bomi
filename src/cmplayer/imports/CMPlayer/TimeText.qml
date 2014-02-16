import QtQuick 2.0
import CMPlayer 1.0 as Cp

Button {
	width: textWidth; height: parent.height; textColor: "white"
	property int msecs: 0
	property int __secs: (msecs/1000.0) | 0
	property bool showMSecs: false
	function getText(msec, point) {
		var text = "";
		if (msec < 0) {
			msec = -msec;
			text += "-"
		}
		var hours = ~~(msec/3600000)
		msec = msec%3600000
		var mins = ~~(msec/60000)
		msec = msec%60000
		var secs = msec/1000;
		text += hours.toString();
		text += mins < 10 ? ":0" : ":";
		text += mins.toString();
		text += secs < 10 ? ":0" : ":";
		text += point ? secs.toFixed(3) : (secs | 0);
		return text;
	}
	text: getText(msecs, showMSecs)
	font { pixelSize: 10; family: Cp.Util.monospace }
}

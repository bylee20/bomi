import QtQuick 2.0 as Q
import bomi 1.0 as B

Q.Text {
    id: textItem
    property bool monospace: true
    property alias content: textItem.text
    font.pixelSize: 10
    font.family: monospace ? B.App.theme.monospace : ""
    verticalAlignment: Q.Text.AlignVCenter
    width: paintedWidth

    function formatTime(msec, point) {
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
        text += secs.toFixed(point ? 3 : 0);
        return text;
    }
    function formatNA(text) {
        return text && text !== "no" ? text : "--";
    }
    function formatNumberNA(n, p) {
        return n <= 0 ? "--" : ("" + (p === undefined ? n : n.toFixed(p)));
    }
    function formatSizeNA(s) {
        return formatNumberNA(s.width) + "x" + formatNumberNA(s.height);
    }
    function formatTrackNumber(i) {
        return i <= 0 ? "-" : ("" + i);
    }
    function formatTrackInfo(info) {
        return formatFraction(info.track.number, info.tracks.length)
    }
    function formatFraction(num, den) {
        return formatTrackNumber(num) + "/" + formatTrackNumber(den)
    }
}

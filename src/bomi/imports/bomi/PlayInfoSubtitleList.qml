import QtQuick 2.0

PlayInfoText {
    id: subs
    property var list: []
    property string name
    function update() {
        var txt = "", count = 0
        for (var i = 0; i < list.length; ++i) {
            var track = list[i];
            if (!track.selected)
                continue
            if (count > 0)
                txt += "\n";
            txt += qsTr("%1 #%2: Codec=%3, Title=%4, Language=%5")
            .arg(name)
            .arg(formatNumber(track.id))
            .arg(formatText(track.codec))
            .arg(formatText(track.title))
            .arg(formatText(track.language))
            ++count;
        }
        text = txt
    }
    onListChanged: subs.update()
    Component.onCompleted: update()
    visible: text.length > 0
    PlayInfoTrack { id: ti; font.pixelSize: subs.parent.fontSize }
}

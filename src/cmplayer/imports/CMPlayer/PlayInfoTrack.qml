import QtQuick 2.0

PlayInfoText {
    id: self
    property string name
    property var info
    function format(name, info) {
        if (name.length <= 0) return ""
        return qsTr("%1 #%2: Codec=%3, Title=%4, Language=%5")
            .arg(name)
            .arg(formatNumber(info.track.id))
            .arg(formatText(info.codec.family))
            .arg(formatText(info.track.title))
            .arg(formatText(info.track.language))
    }
    text: format(name, info)
}

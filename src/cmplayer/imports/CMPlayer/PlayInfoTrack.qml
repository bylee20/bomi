import QtQuick 2.0

PlayInfoText {
    property string name
    property var info
    text: qsTr("%1 #%2: Codec=%3, Title=%4, Language=%5")
            .arg(name)
            .arg(formatNumber(info.track))
            .arg(formatText(info.codec.family))
            .arg(formatText(info.title))
            .arg(formatText(info.language))
}

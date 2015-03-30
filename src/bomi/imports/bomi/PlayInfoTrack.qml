import QtQuick 2.0
import bomi 1.0 as B

PlayInfoText {
    id: self
    property string name: info.track.typeText
    property var info
    function format(name, info) {
        if (name.length <= 0) return ""
        return qsTr("%1 #%2: Codec=%3, Title=%4, Language=%5")
            .arg(name)
            .arg(B.Format.integerNA(info.track.number))
            .arg(B.Format.textNA(info.codec.family))
            .arg(B.Format.textNA(info.track.title))
            .arg(B.Format.textNA(info.track.language))
    }
    content: format(name, info)
}

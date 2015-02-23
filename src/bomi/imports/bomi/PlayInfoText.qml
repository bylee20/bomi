import QtQuick 2.0
import bomi 1.0 as B

B.Text {
    id: item
    textStyle {
        font.pixelSize: fontSize
        color: "yellow"
        styleColor: "black"
        style: Text.Outline;
        monospace: true
    }

    function activationText(s) {
        switch (s) {
        case B.Engine.Unavailable: return qsTr("Unavailable")
        case B.Engine.Deactivated: return qsTr("Deactivated")
        case B.Engine.Activated:   return qsTr("Activated")
        default:                 return ""
        }
    }
    function formatBracket(name, text, inBracket) {
        return name + ": " + text + '[' + inBracket + ']'
    }
}

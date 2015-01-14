import QtQuick 2.2
import bomi 1.0 as Cp

Osd {
    id: osd
    property alias text: text.text
    anchors.fill: parent
    Text {
        id: text
        color: Cp.App.theme.osd.style.color
        font {
            family: Cp.App.theme.osd.style.font
            bold: Cp.App.theme.osd.style.bold
            underline: Cp.App.theme.osd.style.underline
            italic: Cp.App.theme.osd.style.italic
            strikeout: Cp.App.theme.osd.style.strikeout
            pixelSize: height*Cp.App.theme.osd.style.scale
        }
        style: {
            switch (Cp.App.theme.osd.style.style) {
            case Cp.OsdStyleTheme.Outline:
                return Text.Outline;
            case Cp.OsdStyleTheme.Raised:
                return Text.Raised;
            case Cp.OsdStyleTheme.Sunked:
                return Text.Sunken;
            default:
                return Text.Normal;
            }
        }
        styleColor: Cp.App.theme.osd.style.styleColor
        anchors.fill: parent
    }
}

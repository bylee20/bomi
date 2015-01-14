import QtQuick 2.2
import bomi 1.0 as Cp

Osd {
    id: osd
    property alias text: text.text
    Text {
        id: text
        color: Cp.App.theme.osd.color
        font {
            family: Cp.App.theme.osd.font
            bold: Cp.App.theme.osd.bold
            underline: Cp.App.theme.osd.underline
            italic: Cp.App.theme.osd.italic
            strikeout: Cp.App.theme.osd.strikeout
            pixelSize: height*Cp.App.theme.osd.scale
        }
        style: {
            switch (Cp.App.theme.osd.style) {
            case Cp.OsdTheme.Outline:
                return Text.Outline;
            case Cp.OsdTheme.Raised:
                return Text.Raised;
            case Cp.OsdTheme.Sunked:
                return Text.Sunken;
            default:
                return Text.Normal;
            }
        }
        styleColor: Cp.App.theme.osd.styleColor
        anchors.fill: parent
    }
}

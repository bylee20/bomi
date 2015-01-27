import QtQuick 2.2
import bomi 1.0 as B

Osd {
    id: osd
    property alias text: text.text
    anchors.fill: parent
    Text {
        id: text
        color: B.App.theme.osd.style.color
        font {
            family: B.App.theme.osd.style.font
            bold: B.App.theme.osd.style.bold
            underline: B.App.theme.osd.style.underline
            italic: B.App.theme.osd.style.italic
            strikeout: B.App.theme.osd.style.strikeout
            pixelSize: height*B.App.theme.osd.style.scale
        }
        style: {
            switch (B.App.theme.osd.style.style) {
            case B.OsdStyleTheme.Outline:
                return Text.Outline;
            case B.OsdStyleTheme.Raised:
                return Text.Raised;
            case B.OsdStyleTheme.Sunked:
                return Text.Sunken;
            default:
                return Text.Normal;
            }
        }
        styleColor: B.App.theme.osd.style.styleColor
        anchors.fill: parent
    }
}

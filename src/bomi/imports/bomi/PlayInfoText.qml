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
}

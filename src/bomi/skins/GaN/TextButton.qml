import QtQuick 2.0
import bomi 1.0 as B

B.Button {
    width: icon.width + text.contentWidth + spacing; height: 12;
    adjustIconSize: false
    spacing: 2; icon { width: 20; height: 12 }
    text.color: pressed ? "#0ef" : (hovered ? "#0cf" : "white")
    text.width: text.contentWidth
    text.font.pixelSize: 12
    layout: leftIcon
//    function getStateTextColor(hovered, pressed) {  }
//    width: Util.textWidth(showMSecs ? "00:00:00.000" : "00:00:00", text.font.pixelSize, text.font.family);
//    height: text.contentHeight; text.font.pixelSize: 12;
//    text.color: getStateTextColor(hovered || (bind && bind.hovered), pressed || (bind && bind.pressed))
//    onClicked: checked = !checked
//    text.verticalAlignment: Text.AlignVCenter
//    action: bind ? bind.action : ""; action2: bind ? bind.action2 : ""; tooltip: bind ? bind.tooltip : ""

}

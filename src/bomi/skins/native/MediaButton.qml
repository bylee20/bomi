import QtQuick 2.0
import QtQuick.Controls 1.0
import bomi 1.0 as B

ToolButton {
    property string icon
    property string action
    property string fallback: "oxygen"
    iconName: icon
    tooltip: B.App.action(action).text
    iconSource: fallback + "/" + icon + ".png"
    onClicked: B.App.execute(action)
    anchors.verticalCenter: parent.verticalCenter
}

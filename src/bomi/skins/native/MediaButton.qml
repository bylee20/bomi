import QtQuick 2.0
import QtQuick.Controls 1.0
import bomi 1.0 as B

ToolButton {
    id: button
    property alias icon: button.iconName
    property string action
    property string fallback: "oxygen"
    property QtObject __action: B.App.action(action)
    iconName: icon
    tooltip: __action.text
    iconSource: fallback + "/" + icon + ".png"
    onClicked: B.App.execute(action)
    anchors.verticalCenter: parent.verticalCenter
    enabled: __action.enabled
}

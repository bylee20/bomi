import QtQuick 2.0
import bomi 1.0 as B

Item {
    id: item
    property alias icon: icon
    property alias text: _text
    property alias textStyle: _text.textStyle
    property alias background: bg
    property url mask

    onMaskChanged: {
        if (mask.length > 0) {
            area.alpha = 0.01;
            area.source = mask
        } else
            area.alpha = -1.0;
    }

    property string action: ""
    property string action2: ""
    property string tooltip: {
        var left = B.App.description(action);
        var right = B.App.description(action2);
        if (action && action2)
            return makeToolTip(left, right)
        else
            return action ? left : (action2 ? right : "")
    }
    property alias delay: tooltipTimer.interval
    property bool checked: false
    property alias hovered: area.containsMouse
    property alias pressed: area.pressed
    property real paddings: 0
    property alias acceptedButtons: area.acceptedButtons
    property real size: 0
    readonly property int topIcon: 0
    readonly property int leftIcon: 1
    readonly property int centerIcon: 2
    property bool adjustIconSize: true
    property int layout: centerIcon
    property real spacing: 0
    property real emphasize: 0.0

    signal clicked(var mouse)

    width: size; height: size

    function makeToolTip(left, right) {
        return qsTr("Left click: %1\nRight click: %2").arg(left).arg(right)
    }

    function getStateIconName(prefix, hovered, pressed) {
        if (!prefix || !prefix.length)
            return ""
        if (hovered === undefined)
            hovered = item.hovered
        if (pressed === undefined)
            pressed = item.pressed
        if (checked)
            prefix += "-checked";
        prefix += pressed ? "-pressed.png" : ( hovered ? "-hovered.png" : ".png")
        return prefix;
    }

    function formatTrackNumber(info) {
        return B.Format.listNumber(info.track.number, info.tracks.length)
    }

    Rectangle {
        id: bg; anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0)
        Item {
            id: box
            anchors.fill: parent; anchors.margins: item.paddings
            readonly property real sp: icon.visible && text.visible ? item.spacing : 0
            B.ButtonIcon {
                id: icon; smooth: true
                source: getStateIconName(prefix.toString(), hovered, pressed)
                visible: sourceSize.width > 0
                anchors {
                    top:  layout === topIcon  ? parent.top  : undefined
                    left: layout === leftIcon ? parent.left : undefined
                    horizontalCenter: layout === leftIcon ? undefined : parent.horizontalCenter
                    verticalCenter:   layout === topIcon  ? undefined : parent.verticalCenter
                }
                width: {
                    if (!adjustIconSize)
                        return sourceSize.width
                    if (layout === leftIcon)
                        return parent.width - box.sp - text.contentWidth
                    return parent.width
                }
                height: {
                    if (!adjustIconSize)
                        return sourceSize.height
                    if (layout === topIcon)
                        return parent.height - box.sp - text.contentHeight
                    return parent.height
                }
                scale: pressed ? (1.0 - emphasize) : hovered ? (1.0 + emphasize) : 1.0
            }

            B.Text {
                id: _text
                anchors {
                    top:  layout === topIcon  ? parent.bottom : undefined
                    right: layout === leftIcon ? parent.right  : undefined
                    horizontalCenter: layout === leftIcon ? undefined : parent.horizontalCenter
                    verticalCenter:   layout === topIcon  ? undefined : parent.verticalCenter
                }
                width: {
                    if (layout === leftIcon)
                        return (parent.width - icon.width - box.sp)
                    return parent.width
                }
                height: {
                    if (layout === topIcon)
                        return parent.height - icon.height - box.sp
                    return parent.height
                }
                textStyle {
                    horizontalAlignment: layout === leftIcon ? Text.AlignLeft : Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            B.MaskArea {
                id: area; anchors.fill: parent;
                alpha: item.useMask ? 0 : -1
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | (action2 ? Qt.RightButton : 0)
                onReleased: {
                    var action = mouse.button & Qt.RightButton ? item.action2 : item.action
                    if (containsMouse && action)
                        B.App.execute(action)
                }
                onClicked: item.clicked(mouse);
                onExited: B.App.window.hideToolTip();
                onCanceled: B.App.window.hideToolTip()
                Timer {
                    id: tooltipTimer; interval: 1000
                    running: parent.containsMouse && !pressed && tooltip.length
                    onTriggered: B.App.window.showToolTip(parent, Qt.point(parent.mouseX, parent.mouseY), tooltip)
                }
            }
        }
    }
}

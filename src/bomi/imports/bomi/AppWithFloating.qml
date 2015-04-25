import QtQuick 2.0
import bomi 1.0

BaseApp {
    id: root
    readonly property Item rootItem: root
    readonly property alias player: playerItem
    titleBarVisible: true
    Player {
        id: playerItem
        parent: root
        anchors.fill: parent
        MouseArea {
            id: area
            property real pw: -1
            property real ph: -1
            function updatePosX() { floating.setCx(pw < 0 ? initCx : floating.getCx(pw)); pw = width }
            function updatePosY() { floating.setCy(ph < 0 ? initCy : floating.getCy(ph)); ph = height }
            onWidthChanged: updatePosX()
            onHeightChanged: updatePosY()
            Component.onCompleted: {
                controls.parent = floating
                floating.shown = isControlsVisible()
            }
            acceptedButtons: Qt.AllButtons
            anchors.fill: parent
            hoverEnabled: true;
            onPressed: { mouse.accepted = false; root.dismissTools() }
            onReleased: mouse.accepted = false
            onEntered: floating.shown = isControlsVisible()
            onExited: floating.shown = false
            onPositionChanged: {
                var m = App.window.mouse
                m.hidingCursorBlocked = m.isIn(floating)
                floating.shown = isControlsVisible()
            }

            readonly property rect track: Qt.rect(trackingMinX, trackingMinY,
                                                  trackingMaxX - trackingMinX, trackingMaxY - trackingMinY)
            onTrackChanged: floating.shown = isControlsVisible()
            function isControlsVisible() {
                var m = App.window.mouse
                var rect = root.mapToItem(area, track.x, track.y, track.width, track.height)
                if (!m.isIn(area, Qt.rect(rect.x, rect.y, rect.width, rect.height)))
                    return false
                if (App.theme.controls.showOnMouseMoved)
                    return m.cursor && m.isIn(area)
                return m.isIn(floating)
            }

            MouseArea {
                id: floating
                function clamp(min, v, max) { return Math.max(Math.min(max, v), min); }
                function setCx(cx) { x = clamp(0, cx*parent.width - width/2, parent.width - width) }
                function setCy(cy) { y = clamp(0, cy*parent.height - height/2, parent.height - height) }
                function getCx(bg) { return (x+width/2)/bg; }
                function getCy(bg) { return (y+height/2)/bg; }

                property bool shown: true
                width: controls.width; height: controls.height
                drag.target: floating; drag.axis: Drag.XAndYAxis
                drag.minimumX: 0; drag.maximumX: root.width-width
                drag.minimumY: 0; drag.maximumY: root.height-height
                states: State {
                    name: "hidden"; when: !floating.shown
                    PropertyChanges { target: floating; opacity: 0.0 }
                    PropertyChanges { target: floating; visible: false }
                }
                transitions: Transition {
                    reversible: true; to: "hidden"
                    SequentialAnimation {
                        NumberAnimation { property: "opacity"; duration: 200 }
                        PropertyAction { property: "visible" }
                    }
                }
                Component.onCompleted: {
                    App.registerToAccept(floating, App.DoubleClickEvent)
                }
            }
            Connections {
                target: App.window.mouse
                onCursorChanged: floating.shown = area.isControlsVisible()
            }
        }
    }

    property Item controls

    property real initCx: 0.5
    property real initCy: 0.0
    Component.onCompleted: {
        Settings.open(root.name)
        initCx = Settings.getReal("cx", 0.5)
        initCy = Settings.getReal("cy", 0.0)
        Settings.close()
    }
    Component.onDestruction: {
        Settings.open(root.name)
        Settings.set("cx", floating.getCx(area.width))
        Settings.set("cy", floating.getCy(area.height))
        Settings.close()
    }
}

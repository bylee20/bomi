import QtQuick 2.0
import bomi 1.0

BaseApp {
    id: root
    readonly property alias player: playerItem
    titleBarVisible: topControls.height <= 0
    toolStyle {
        topMargin: overlaps ? 0 : topControls.height
        bottomMargin: overlaps ? 0 : bottomControls.height
        topMarginForInteraction: topControls.height
        bottomMarginForInteraction: bottomControls.height
    }

    Player {
        id: playerItem
        width: parent.width
        height: overlaps ? root.height : root.height - bottomControls.height - topControls.height
        topPadding: overlaps ? topControls.height + topControls.y : 0
        bottomPadding: overlaps ? bottomControls.height - bottomControls.y : 0

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onPressed: { mouse.accepted = false; root.dismissTools() }
            onReleased: mouse.accepted = false
            onEntered: d.updateShown();
            onExited: d.updateShown();
            onPositionChanged: d.updateShown();
        }

        Component {
            id: blur
            SimpleBlur {
                source: blurSource; textureSize: blurSource.textureSize
                ShaderEffectSource {
                    id: blurSource; anchors.fill: parent
                    sourceItem: playerItem.screen; visible: false
                    sourceRect: rect
                    textureSize: Qt.size(width / 2, height / 2)
                }
            }
        }

        Loader {
            readonly property rect rect: Qt.rect(x, y, width, height)
            sourceComponent: blurBackground && topControls.height > 0.01 ? blur : undefined
            width: parent.width; height: playerItem.topPadding
        }

        Loader {
            readonly property rect rect: Qt.rect(x, y, width, height)
            sourceComponent: blurBackground && bottomControls.height > 0.01 ? blur : undefined
            width: parent.width; height: playerItem.bottomPadding
            anchors.bottom: parent.bottom
        }
    }

    property Item topControls: Item {}
    property Item bottomControls: Item {}
    property bool overlaps: App.window.fullscreen
    property bool blurBackground: false

    onOverlapsChanged: d.updateShown()

    Connections {
        target: App.window.mouse
        onCursorChanged: d.updateShown();
    }

    QtObject {
        id: d
        property bool shown: true
        property bool completed: false
        readonly property bool containsMouse: topCatcher.containsMouse
                                              || btmCatcher.containsMouse
        readonly property rect track: Qt.rect(trackingMinX, trackingMinY,
                                              trackingMaxX - trackingMinX, trackingMaxY - trackingMinY)
        onTrackChanged: updateShown()
        function updateShown() { shown = isControlsVisible(); }
        function isControlsVisible() {
            if (!overlaps)
                return true;
            var m = App.window.mouse
            var rect = root.mapToItem(area, track.x, track.y, track.width, track.height)
            if (!m.isIn(playerItem, Qt.rect(rect.x, rect.y, rect.width, rect.height)))
                return false
            if (App.theme.controls.showOnMouseMoved)
                return m.cursor && m.isIn(area)
            return m.isIn(btmCatcher) || m.isIn(topCatcher)
        }
        onContainsMouseChanged: {
            updateShown()
            App.window.mouse.hidingCursorBlocked = containsMouse
        }
    }

    states: State {
        name: "hidden"; when: !d.shown
        PropertyChanges { target: topControls; y: -topControls.height }
        PropertyChanges { target: bottomControls; y: bottomControls.height }
    }

    transitions: Transition {
        reversible: true; to: "hidden"
        ParallelAnimation {
            NumberAnimation { target: topControls; property: "y"; duration: 200 }
            NumberAnimation { target: bottomControls; property: "y"; duration: 200 }
        }
    }

    MouseArea {
        id: topCatcher; z: player.z + 1;
        width: parent.width; height: topControls.height
        anchors.top: parent.top; hoverEnabled: true
        Component.onCompleted: App.registerToAccept(topCatcher, App.DoubleClickEvent)
    }

    MouseArea {
        id: btmCatcher; z: player.z + 1;
        width: parent.width; height: bottomControls.height
        anchors.bottom: parent.bottom; hoverEnabled: true
        Component.onCompleted: App.registerToAccept(btmCatcher, App.DoubleClickEvent)
    }

    onBottomControlsChanged: {
        if (d.completed) {
            bottomControls.parent = btmCatcher
            d.updateShown()
        }
    }

    onTopControlsChanged: {
        if (d.completed) {
            topControls.parent = topCatcher
            d.updateShown()
        }
    }

    Component.onCompleted: {
        bottomControls.parent = btmCatcher
        topControls.parent = topCatcher
        d.updateShown();
        d.completed = true
    }
}

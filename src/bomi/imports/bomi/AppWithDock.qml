import QtQuick 2.0
import bomi 1.0

BaseApp {
    id: root
    player: playerItem

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        onPressed: mouse.accepted = false
        onReleased: mouse.accepted = false
        onEntered: catcher.update();
        onExited: catcher.update();
        onPositionChanged: catcher.update();
    }

    Player {
        id: playerItem
        width: parent.width
        height: App.window.fullscreen ? root.height : root.height - controls.height
    }

    property Item controls: Item {}

    function isControlsVisible() {
        if (!App.window.fullscreen)
            return true;
        var m = App.window.mouse
        if (App.theme.controls.showOnMouseMoved)
            return m.cursor && m.isIn(area)
        return m.isIn(catcher)
    }

    Connections {
        id: conn
        function update() {
            catcher.update()
            if (target.fullscreen)
                player.bottomPadding = catcher.height
            else
                player.bottomPadding = 0
        }

        target: App.window
        onFullscreenChanged: update()
    }

    Connections {
        target: App.window.mouse
        onCursorChanged: catcher.update();
    }

    MouseArea {
        id: catcher; z: player.z+1;
        width: parent.width
        height: controls.height
        anchors.bottom: parent.bottom
        hoverEnabled: true
        property bool shown: true
        function update() { shown = root.isControlsVisible() }

        onShownChanged: {
            if (App.window.fullscreen && !shown)
                sliding.start()
            else
                controls.y = 0
        }

        onContainsMouseChanged: {
            update()
            var m = App.window.mouse
            m.hidingCursorBlocked = containsMouse
        }
        NumberAnimation {
            id: sliding; target: controls; property: "y";
            duration: 200; from: 0; to: controls.height
        }
    }
    Component.onCompleted: {
        controls.parent = catcher
        catcher.update();
    }
}

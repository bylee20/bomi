import QtQuick 2.0
import bomi 1.0

BaseApp {
    id: root
    player: playerItem

    Player {
        id: playerItem
        width: parent.width
        height: App.window.fullscreen ? root.height : root.height - controls.height
    }

    property Item controls: Item {}

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

    MouseArea {
        id: catcher; z: player.z+1;
        width: parent.width
        height: controls.height
        anchors.bottom: parent.bottom
        hoverEnabled: true
        function update() {
            if (App.window.fullscreen && !catcher.containsMouse)
                sliding.start()
            else
                controls.y = 0
        }
        onContainsMouseChanged: update()
        NumberAnimation {
            id: sliding; target: controls; property: "y";
            duration: 200; from: 0; to: controls.height
        }
    }
    Component.onCompleted: {
        controls.parent = catcher
        conn.update()
    }
}

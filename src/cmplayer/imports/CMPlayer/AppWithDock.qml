import QtQuick 2.0
import CMPlayer 1.0

Item {
    id: root
    property Player player: Player {}
    property Item controls: Item {}
    Binding { target: player; property: "width"; value: root.width }
    Binding {
        target: player; property: "height";
        value: Util.fullScreen ? root.height : root.height - controls.height
    }
    Connections {
        id: conn
        function update() {
            catcher.update()
            if (Util.fullScreen)
                player.bottomPadding = catcher.height
            else
                player.bottomPadding = 0
        }

        target: Util
        onFullScreenChanged: update()
    }

    MouseArea {
        id: catcher; z: player.z+1;
        width: parent.width
        height: controls.height
        anchors.bottom: parent.bottom
        hoverEnabled: true
        function update() {
            if (Util.fullScreen && !catcher.containsMouse)
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
        player.parent = root;
        controls.parent = catcher
        conn.update()
    }
}

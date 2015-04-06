import QtQuick 2.0
import bomi 1.0

Item {
    id: root
    property Player player
    property string name
    property size minimumSize: Qt.size(400, 300)
    readonly property Engine engine: App.engine
}

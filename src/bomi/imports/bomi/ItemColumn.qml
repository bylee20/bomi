import QtQuick 2.0

QtObject {
    id: obj
    property int width: 0
    property string title
    property string role
    property int index: -1
    property Item header
    property Item separator
    property real position: header ? header.x : 0
}

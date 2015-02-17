import QtQuick 2.0
import QtQuick 2.0 as Q
import bomi 1.0

Item { id: view

    property real rowHeight: 16
    property bool headerVisible: true
    property alias count: list.count
    readonly property real scrollArea: 10
    property real margins: 15
    property list<ItemColumn> columns
    property alias model: list.model
    readonly property real contentWidth: {
        var width = 0
        for (var i=0; i<columns.length; ++i)
            width += columns[i].width + columns[i].separator.width
        return width
    }
    property Component rowDelegate: Component {
        Rectangle {
            height: rowHeight
            color: current ? Qt.rgba(0, 0.73, 1, .5)
                    : selected ? Qt.rgba(0, 0.93, 1.0, 0.5)
                    : alternating ? Qt.rgba(0.0, 0.0, 0.0, 0.5) : Qt.rgba(0.25, 0.25, 0.25, 0.5)
        }
    }
    property Component itemDelegate: Component { Text { text: value } }
    property Component headerItemDelegate: Component {
        Item {
            Q.Text {
                text: column.title
                anchors { fill: parent; leftMargin: 5; rightMargin: 5 }
                color: "black"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
                font.pixelSize: 11
            }
        }
    }
    property Component headerSeparatorDelegate: Component {
        Item {
            width: 10; height: parent.height
            Rectangle { anchors.centerIn: parent; width: 1; height: parent.height*0.6; color: "black" }
        }
    }
    property alias selectedIndex: list.selectedIndex
    property alias currentIndex: list.currentIndex

    signal activated(int index)

    function isVaildIndex(index) {
        return 0 <= index && index < list.count
    }

    anchors { fill: parent; margins: margins }
    Component.onCompleted: d.updateColumnsPosition()

    QtObject { id: d
        function syncContentX(self, sync) {
            if (self.contentX < 0)
                self.contentX = 0
            else {
                var max = Math.max(0, self.contentWidth - self.width)
                if (self.contentX > max)
                    self.contentX = max
            }
            if (self.movingHorizontally && sync.visible)
                sync.contentX = self.contentX
        }
        function updateColumnsPosition() {
            var pos = 0
            for (var i=0; i<columns.length; ++i) {
                var column = columns[i]
                if (column.header) {
                    column.header.x = pos
                    pos += column.width + column.separator.width
                }
            }
        }
    }

    Rectangle { id: frame; anchors.fill: parent; border { color: "#fff"; width: 1 } color: Qt.rgba(0, 0, 0, 0.4) }

    Rectangle { id: headerBox
        color: Qt.rgba(1, 1, 1, 0.7)
        visible: headerVisible
        height: 15; anchors { top: parent.top; left: parent.left; right: parent.right; margins: frame.border.width }

        Flickable { id: headerFlickable
            anchors.fill: parent; clip: true
            contentWidth: view.contentWidth; contentHeight: height
            flickableDirection: Flickable.HorizontalFlick
            Item { id: headerContainer
                width: view.contentWidth; height: parent.height
                Repeater {
                    model: view.columns
                    Loader {
                        id: headerItemLoaders
                        readonly property ItemColumn column: modelData
                        sourceComponent: headerItemDelegate
                        width: column.width; height: parent.height
                        onLoaded: {
                            column.header = headerItemLoaders
                            column.index = index
                            column.separator = sep
                        }
                        Loader {
                            id: sep
                            sourceComponent: headerSeparatorDelegate
                            x: parent.width; height: parent.height
                            onXChanged: { column.width = x; d.updateColumnsPosition() }

                            MouseArea {
                                anchors.fill: parent; preventStealing: true
                                drag { target: sep; axis: Drag.XAxis; minimumX: 0; maximumX: 2000 }
                            }
                        }
                    }
                }
            }
            onContentXChanged: d.syncContentX(headerFlickable, list)
        }
    }

    ListView {
        id: list
        property int selectedIndex: -1
        currentIndex: -1
        anchors { fill: frame; margins: frame.border.width; topMargin: headerBox.visible ? headerBox.height+1 : 1 } clip: true
        contentWidth: view.contentWidth
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        delegate: Item {
            readonly property int row: index
            readonly property var itemModel: model
            width: 2000; height: rowLoader.implicitHeight
            Loader {
                id: rowLoader
                readonly property bool current: currentIndex == index
                readonly property bool selected: selectedIndex == index
                readonly property bool alternating: index & 1
                width: parent.width
                sourceComponent: rowDelegate
            }
            Repeater {
                model: view.columns
                Loader {
                    readonly property ItemColumn column: modelData
                    readonly property var value: itemModel[column.role]
                    readonly property int index: row
                    readonly property bool current: currentIndex == index
                    readonly property bool selected: selectedIndex == index
                    readonly property bool alternating: index & 1
                    sourceComponent: itemDelegate
                    x: column.position+5; width: column.width-10; height: parent.height
                }
            }
        }
        onContentXChanged: d.syncContentX(list, headerFlickable)
        Component.onCompleted: {
            App.registerToAccept(list, App.DoubleClickEvent | App.KeyEvent);
        }

        function getIndex(y) {
            var pos = list.contentItem.mapFromItem(list, 0.5, y)
            return list.indexAt(pos.x, pos.y);
        }


        Keys.onPressed: {
            event.accepted = true
            var idx = -1
            switch (event.key) {
            case Qt.Key_Up:
                idx = selectedIndex - 1
                break
            case Qt.Key_Down:
                idx = selectedIndex + 1
                break;
            case Qt.Key_PageUp:
            case Qt.Key_PageDown:
                var count = getIndex(height-1) - getIndex(1)
                if (count <= 0)
                    return
                if (Qt.Key_PageUp === event.key)
                    idx = selectedIndex - count
                else
                    idx = selectedIndex + count
                break
            case Qt.Key_End:
                idx = list.count - 1
                break
            case Qt.Key_Home:
                idx = 0
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                if (isVaildIndex(selectedIndex))
                    activated(selectedIndex)
                return
            default:
                event.accepted = false
                return;
            }
            if (list.count < 1)
                return;
            idx = Math.max(Math.min(idx, list.count - 1), 0);
            if (idx != selectedIndex && isVaildIndex(idx)) {
                selectedIndex = idx
                positionViewAtIndex(idx, ListView.Visible)
            }
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                var index = list.getIndex(mouse.y)
                if (index !== -1)
                    list.selectedIndex = index
            }
            onDoubleClicked: {
                var index = list.getIndex(mouse.y)
                if (index !== -1)
                    view.activated(index)
            }
        }

        ScrollBar {
            anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
            }
            width: 15
            gap: 3
            border.color: Qt.rgba(0.5, 0.5, 0.5, 0.5)
            radius: 2
            border.width: 1
            target: parent
            color: Qt.rgba(1.0, 1.0, 1.0, 0.8)
        }
    }
}

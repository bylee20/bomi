import QtQuick 2.0
import QtQuick 2.0 as Q
import bomi 1.0

Item {
    id: view
    property real rowHeight: 30
    property bool headerVisible: true
    property alias count: list.count
    property int titlePadding: 0
    readonly property real scrollArea: 10
    property real margins: 0
    property list<ItemColumn> columns
    readonly property alias blockHiding: d.blockHiding
    readonly property alias titleSeparator: __titleSeparator
    property alias model: list.model
    readonly property real contentWidth: {
        var width = 0
        for (var i=0; i<columns.length; ++i)
            width += columns[i].width + columns[i].separator.width
        return width
    }
    property Component rowDelegate: Item { height: rowHeight }
    property Component itemDelegate: Q.Text { text: value }
    property Component headerItemDelegate: Item {
        Rectangle {
            id: headerMarker
            width: 4; height: width
            anchors.verticalCenter: parent.verticalCenter
            radius: width*0.5; color: "white"
            visible: column.index === 1
        }
        Q.Text {
            text: column.title
            anchors { fill: parent; leftMargin: 5 + (headerMarker.visible ? 4 : 0); rightMargin: 5 }
            color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
            font.pixelSize: 12
        }
    }
    property Component headerSeparatorDelegate: Item {
        width: 4; height: parent.height
        Rectangle {
            anchors.centerIn: parent;
            width: 4; height: width
            radius: width*0.5; color: "white"
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
        readonly property real pad: 15
        property bool blockHiding: scrollBar.pressed
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
        function reposition(highlighter, target) {
            if (!target) {
                highlighter.height = 0
                return
            }
            var p = target.mapToItem(view, 0, target.y).y
            var h = target.height
            if (p < list.y) {
                h -= (list.y - p)
                p += (list.y - p)
            } else if (p + h > list.y + list.height)
                h = (list.y + list.height) - p
            highlighter.height = h
            highlighter.y = p
        }
    }

    Rectangle { id: frame; anchors.fill: parent; color: Qt.rgba(0, 0, 0, 0.55) }

    Rectangle { id: headerBox
        color: Qt.rgba(0, 0, 0, 0.3)
        height: titlePadding + (headerFlickable.visible ? headerFlickable.height : 0) + __titleSeparator.height;
        anchors { top: parent.top; left: parent.left; right: parent.right }

        Flickable {
            id: headerFlickable
            x: d.pad
            visible: headerVisible
            width: parent.width - 2*x; height: 16 + x
            anchors { top: parent.top; topMargin: titlePadding }
            clip: true
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
        Rectangle {
            id: __titleSeparator; height: 1
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        }
    }

    Rectangle {
        id: current
        width: parent.width
        color: Qt.rgba(1, 1, 1, 0.3)
        function reposition() { d.reposition(current, list.curItem) }
        Component.onCompleted: reposition()
    }

    Rectangle {
        id: selection
        width: parent.width
        color: Qt.rgba(1, 1, 1, 0.2)
        function reposition() { d.reposition(selection, list.selectedItem) }
        Component.onCompleted: reposition()
    }

    ListView {
        id: list
        property int selectedIndex: -1
        property Item selectedItem: null
        property Item curItem: null

        onCurItemChanged:  current.reposition()
        onSelectedItemChanged: selection.reposition()
        onContentYChanged: {selection.reposition(); current.reposition()}
        onHeightChanged: {selection.reposition(); current.reposition()}
        highlightMoveDuration: 500

        highlight: Item { }
        currentIndex: -1
        anchors {
            fill: frame; leftMargin: d.pad - 2; rightMargin: d.pad + 2
            topMargin: (headerBox.visible ? headerBox.height : 0)
        }
        clip: true
        contentWidth: view.contentWidth
        flickableDirection: Flickable.HorizontalAndVerticalFlick
        footer: Item { width: parent.width; height: d.pad - 4 }
        header: Item { width: parent.width; height: d.pad - 4 }

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
                onSelectedChanged: {
                    if (selected)
                        list.selectedItem = this
                }
                onCurrentChanged: {
                    if (current)
                        list.curItem = this
                }
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
                    x: column.position; width: column.width - 5; height: rowLoader.height
                }
            }
            Item {
                id: __footer; anchors.bottom: parent.bottom
                x: list.contentX; height: 1; width: list.width;
                Image {
                    source: "qrc:/img/list-sep.png"
                    visible: row < (list.model.length - 1)
                    anchors { fill: parent; leftMargin: 30; rightMargin: 30 }
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
        function getItem(x, y) {
            var pos = list.contentItem.mapFromItem(list, x, y)
            return list.itemAt(pos.x, pos.y);
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

        function calcVelocity(angle, smooth, pos, min, max) {
            var v = angle
            if (v > 0 && pos > min)
                return Math.max(v - smooth, maximumFlickVelocity / 4)
            else if (v < 0 && pos < max)
                return Math.min(v - smooth, -maximumFlickVelocity / 4)
            return 0
        }

        function wheelScroll(angle, pixel) {
            if (pixel.x !== 0 || pixel.y !== 0) {
                contentX -= pixel.x
                contentY -= pixel.y
                returnToBounds()
            } else {
                var vx = calcVelocity(angle.x, horizontalVelocity, contentX,
                                      0, contentWidth - list.width)
                var vy = calcVelocity(angle.y, verticalVelocity, contentY,
                                      -headerItem.height, contentHeight - list.height - footerItem.height)
                flick(vx, vy)
            }
        }

        MouseArea {
            z: -1
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
            onWheel: list.wheelScroll(wheel.angleDelta, wheel.pixelDelta)
        }
    }
    ScrollBar {
        id: scrollBar
        width: 15; gap: 3
        anchors { top: list.top; bottom: list.bottom; right: parent.right }
        target: list; radius: 2; color: Qt.rgba(1.0, 1.0, 1.0, 0.8)
        border { color: Qt.rgba(0.5, 0.5, 0.5, 0.5); width: 1 }
    }
}

import QtQuick 2.0
import bomi 1.0 as B

B.ModelView {
    id: root
    model: B.App.playlist
    margins: 0

    QtObject {
        id: d
    }
    Text {
        id: _name
        font { pixelSize: 15 }
        visible: false
        text: "DUMMY TEXT FOR HEIGHT"
        readonly property real h: contentHeight
        function getWidth(text) {
            return B.App.textWidth(text, font.pixelSize, font.family)
        }
    }
    Text {
        id: _location
        readonly property bool show: B.App.theme.playlist.showLocation
        readonly property real h: contentHeight
        font { pixelSize: 10; family: B.App.theme.monospace }
        visible: false
        text: "DUMMY TEXT FOR HEIGHT"
        function getWidth(text) {
            return B.App.textWidth(text, font.pixelSize, font.family)
        }
    }

    headerVisible: false
    rowHeight: _name.contentHeight + (_location.show ? _location.contentHeight : 0) + 5;

    currentIndex: model.loaded
    function contentWidth() {
        var max = 0;
        for (var i=0; i<root.count; ++i) {
            var number = _name.getWidth(model.number(i))
            var name = _name.getWidth(model.name(i))
            if (_location.show) {
                var loc = _location.getWidth(model.location(i))
                max = Math.max(number + name, loc, max);
            } else
                max = Math.max(number + name, max);
        }
        return max+30
    }

    Connections {
        target: root.model;
        onSelectedChanged: root.selectedIndex = target.selected
    }
    onSelectedIndexChanged: model.selected = root.selectedIndex

    onCountChanged: column.width = contentWidth()
    columns: B.ItemColumn { title: "Name"; role: "name"; width: 200; id: column}

    onActivated: model.play(index)
    itemDelegate: Item {
        Column {
            width: parent.width
            Text {
                anchors { margins: 5; left: parent.left; right: parent.right }
                font {
                    family: _name.font.family; pixelSize: _name.font.pixelSize
                    italic: current; bold: current
                }
                verticalAlignment: Text.AlignVCenter; height: _name.h
                color: "white"; text: value; elide: Text.ElideRight
            }
            Text {
                visible: _location.show
                anchors { margins: 5; left: parent.left; right: parent.right }
                font: _location.font
                width: parent.width; height: _location.h; verticalAlignment: Text.AlignTop
                color: "white"; text: model.location(index); elide: Text.ElideRight
            }
        }
    }
}

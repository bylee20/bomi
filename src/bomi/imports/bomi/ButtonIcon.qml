import QtQuick 2.0

Image {
    id: icon; smooth: sourceSize != Qt.size(width, height)
    property url prefix
    visible: sourceSize.width > 0
}

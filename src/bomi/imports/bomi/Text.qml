import QtQuick 2.0
import QtQuick 2.0 as Q
import bomi 1.0
import bomi 1.0 as B

Item {
    property TextStyle textStyle: TextStyle { }
    property alias contentWidth: q.contentWidth
    property alias contentHeight: q.contentHeight
    property alias content: q.text
    implicitHeight: q.implicitHeight
    implicitWidth: q.implicitWidth
    width: contentWidth
    Q.Text {
        id: q
        anchors.fill: parent
        font: textStyle.font
        verticalAlignment: textStyle.verticalAlignment
        horizontalAlignment: textStyle.horizontalAlignment
        style: textStyle.style
        styleColor: textStyle.styleColor
        color: textStyle.color
        textFormat: textStyle.format
        elide: textStyle.elide
        wrapMode: textStyle.wrapMode
    }
}

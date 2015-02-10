import QtQuick 2.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0

QtObject {
    property TimeSlider control
    property string name
    property int time
    property QtObject chapter: QtObject { }
    property Component marker
}

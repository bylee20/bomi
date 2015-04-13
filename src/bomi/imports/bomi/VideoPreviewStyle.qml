import QtQuick 2.0
import bomi 1.0

QtObject {
    property int onTop: -1
    property real separation: 5
    property real height: Alg.clamp(App.window.height * 0.2, 100, 200)
}

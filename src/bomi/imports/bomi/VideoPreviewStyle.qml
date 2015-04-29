import QtQuick 2.0
import bomi 1.0

QtObject {
    property int onTop: -1
    property real separation: 5
    readonly property int height: Alg.clamp(App.window.height * App.theme.controls.previewSize,
                                             App.theme.controls.previewMinimumSize,
                                             App.theme.controls.previewMaximumSize)
}

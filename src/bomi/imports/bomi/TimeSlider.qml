import QtQuick 2.0
import bomi 1.0

Slider {
    id: seeker
    property Component markerStyle
    property alias time: seeker.value
    property TimeDuration bind
    property bool toolTip: !bind
    property int target: -1

    onBindChanged: {
        if (bind)
            bind.time = Qt.binding(function ( ) { return mouseArea.time; })
    }

    Repeater {
        model: d.engine.chapters
        Loader {
            readonly property Chapter chapter: modelData
            readonly property Slider control: seeker
            function seek() { seeker.time = chapter.time; }
            x: seeker.width * chapter.rate
            sourceComponent: markerStyle
            function updateTarget() {
                if (item.hovered && seeker.bind)
                    seeker.target = chapter.time
                else
                    seeker.target = -1
            }
            onItemChanged: {
                if (item.hovered !== undefined)
                    item.hoveredChanged.connect(updateTarget)
                if (item.clicked !== undefined)
                    item.clicked.connect(seek)
            }
        }
    }

    QtObject {
        id: d;
        readonly property Engine engine: App.engine
        property bool ticking: false
        function target(x) { return (min + (x/seeker.width)*(max - min)); }
        function sync() {
            seeker.min = engine.begin
            seeker.max = engine.end
            seeker.value = engine.time
        }
    }

    Connections {
        target: d.engine
        onTick: { d.ticking = true; time = d.engine.time; d.ticking = false; }
        onEndChanged: { d.ticking = true; d.sync(); d.ticking = false; }
        onBeginChanged: { d.ticking = true; d.sync(); d.ticking = false; }
    }
    onValueChanged: { if (!d.ticking) d.engine.seek(value); }
    Component.onCompleted: d.sync()

    MouseArea {
        id: mouseArea
        visible: bind || toolTip
        anchors.fill: parent
        hoverEnabled: true
        property bool moving: pressed || containsMouse
        property int time: target >= 0 ? target : moving ? d.target(mouseX) : d.engine.time
        Text { id: t }
        onMovingChanged: if (toolTip && !moving) App.window.hideToolTip()
        onPositionChanged: {
            var val = d.target(mouse.x)
            if (toolTip)
                App.window.showToolTip(seeker, mouse.x, mouse.y,
                                       Format.time(val) + "/" + Format.time(max))
            if (pressed)
                value = val
        }
        onPressed: { value = d.target(mouse.x) }
    }
}

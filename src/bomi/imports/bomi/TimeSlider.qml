import QtQuick 2.0
import bomi 1.0

Slider {
    id: seeker
    property Component markerStyle
    property alias time: seeker.value
    property TimeDuration bind
    property bool toolTip: !bind
    property VideoPreviewStyle preview: VideoPreviewStyle { }
    property int target: -1
    enabled: d.e.seekable
    acceptsWheel: true
    z: 1
    __hpressed: mouseArea.pressed
    __hhovered: mouseArea.containsMouse
    ahead: d.e.cache.time

    Rectangle {
        id: pv

        property bool onTop: true
        property bool shown: App.theme.controls.showPreviewOnMouseOverSeekBar
                             && d.e.preview.hasVideo && d.e.seekable
                             && d.e.running && mouseArea.containsMouse && x >= 0
        parent: App.window.z10
        x: -1; width: (d.e.preview.width + 4); height: (d.e.preview.height + 4)
        visible: false; opacity: 0.0; z: 10; border { width: 1; color: "black" }
        states: State {
            name: "shown";
            PropertyChanges { target: pv; visible: true; opacity: 1.0 }
        }
        transitions: Transition {
            reversible: true; to: "shown"
            SequentialAnimation {
                PropertyAction { property: "y" }
                PropertyAction { property: "visible" }
                NumberAnimation { property: "opacity"; duration: 200 }
            }
        }
        Component.onCompleted: {
            d.e.preview.parent = this
            d.e.preview.width = Qt.binding(function() { return (this.height * this.aspectRatio) | 0; });
            d.e.preview.height = Qt.binding(function() { return preview.height | 0; });
            d.e.preview.anchors.centerIn = Qt.binding(function() { return pv; });
        }
        onShownChanged: {
            if (shown) { onTop = updateOnTop(); y = yPos(); }
            state = shown ? "shown" : "hidden"
        }
        function updateOnTop() {
            if (preview.onTop < 0)
                return seeker.mapToItem(null, 0, seeker.y).y > App.window.height * 0.5
            else
                return preview.onTop
        }
        function yPos() {
            var l = onTop ? (-height - preview.separation) : (preview.separation + seeker.height)
            return seeker.mapToItem(null, 0, l).y | 0
        }

        function repos(mx) { x = Alg.clamp(mx - pv.width * 0.5, 0, App.window.width - pv.width) | 0 }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: pv.onTop ? (parent.height - height - 10) : 10; z: 1
            width: pvTime.width + 10; height: pvTime.height + 10
            color: Qt.rgba(0, 0, 0, 0.5)
            TimeText {
                id: pvTime
                anchors.centerIn: parent
                height: contentHeight; width: contentWidth
                textStyle { color: "white"; font.pixelSize: preview.height * 0.11 }
            }
        }
    }
    onBindChanged: {
        if (bind)
            bind.time = Qt.binding(function ( ) { return mouseArea.time; })
    }

    Repeater {
        model: d.e.chapters
        Loader {
            readonly property Chapter chapter: modelData
            readonly property Slider control: seeker
            property bool hCenter: true
            property bool vCenter: false
            function seek() { seeker.time = chapter.time; }
            x: seeker.width * chapter.rate - (hCenter ? width * 0.5 : 0)
            y: vCenter ? (control.height - height) * 0.5 : 0
            sourceComponent: markerStyle
            function updateTarget() {
                if (item.hovered)
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
        readonly property Engine e: App.engine
        property bool ticking: true
        function target(x) { return (min + (x/seeker.width)*(max - min)); }
        function sync() {
            seeker.min = e.begin
            seeker.max = e.end
            seeker.value = e.time
        }
    }

    Connections {
        target: d.e
        onTick: { d.ticking = true; time = d.e.time; d.ticking = false; }
        onEndChanged: { d.ticking = true; d.sync(); d.ticking = false; }
        onBeginChanged: { d.ticking = true; d.sync(); d.ticking = false; }
    }
    onValueChanged: { if (!d.ticking) d.e.seek(value); }
    Component.onCompleted: { d.sync(); d.ticking = false; }

    MouseArea {
        id: mouseArea
        visible: bind || toolTip
        anchors.fill: parent
        hoverEnabled: true
        property bool moving: pressed || containsMouse
        property bool hv: false
        property int time: target >= 0 ? target : moving ? d.target(mouseX) : d.e.time
        onMovingChanged: if (toolTip && !moving) App.window.hideToolTip()
        onPositionChanged: {
            var val = d.target(mouse.x)
            if (toolTip)
                App.window.showToolTip(seeker, mouse.x, mouse.y,
                                       Format.time(val) + "/" + Format.time(max))
            if (App.theme.controls.showPreviewOnMouseOverSeekBar) {
                d.e.preview.parent = pv
                pv.repos(mapToItem(null, mouse.x, 0).x)
                d.e.preview.rate = d.e.rate_ms(val);
                pvTime.time = val
            }
            if (pressed)
                value = val
        }
        onPressed: { value = d.target(mouse.x) }
    }
}

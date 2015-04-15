// CREDIT: central icon designed by Ivan(Kotus works)
//         https://plus.google.com/u/1/117118228830713086299/posts

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.0
import bomi 1.0 as B

B.AppWithFloating {
    id: skin;
    name: "net.xylosper.bomi.one"
    readonly property QtObject engine: B.App.engine

    B.Text {
        id: text // dummy for text formatting
        visible: false
    }

    controls: Item {
        width: 130; height: 130;

        MouseArea {
            id: mouse
            anchors.fill: parent
            onPressed: mouse.accepted = false
            hoverEnabled: true
            propagateComposedEvents: true
            onContainsMouseChanged: {
                if (containsMouse) {
                    shrink.running = false
                    expand.running = true
                    cursorChecker.start()
                }
            }

            Timer {
                id: cursorChecker
                interval: 1500
                repeat: true
                onTriggered: {
                    if (!B.App.window.mouse.isIn(mouse)) {
                        expand.running = false
                        shrink.running = true
                        cursorChecker.stop()
                    }
                }
            }
        }

        Rectangle {
            id: box
            anchors.fill: parent
            opacity: back.scale
            radius: 10
            property real g: 1
            color: Qt.rgba(g, g, g, 0.3)
        }

        Item {
            id: back
            anchors.fill: parent
            anchors.margins: 10
            scale: 0
            NumberAnimation {
                id: expand; target: back; property: "scale"; to: 1.0
                duration: 100; easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                id: shrink; target: back; property: "scale"; to: 0.0
                duration: 500; easing.type: Easing.InOutQuad
            }

            IconButton {
                anchors { top: parent.top; left: parent.left }
                icon.source: "cb-audio.png";
                action: "audio/track/next"; action2: "audio/track"
            }

            IconButton {
                anchors { top: parent.top; right: parent.right }
                icon.source: "cb-sub.png"
                action: "subtitle/track/next"; action2: "subtitle/track"
            }
            IconButton {
                anchors { bottom: parent.bottom; right: parent.right }
                icon.source: "cb-pl.png"
                action: "tool/playlist/toggle"; action2: "tool/playlist"
            }
            IconButton {
                anchors { bottom: parent.bottom; left: parent.left }
                icon.source: "cb-hl.png"
                action: "tool/history/toggle"; action2: "tool/history"
            }
        }



        Image {
            id: bg
            width: 100; height: 100;
            anchors.centerIn: parent
            readonly property point c: Qt.point(0.5*width, 0.5*height)
            source: "base.png"
            Image {
                anchors.fill: parent
                source: "trace.png"
                visible: !engine.stopped
                RotationAnimation on rotation {
                    loops: Animation.Infinite
                    from: 0; to: 360
                    running: !engine.paused
                    duration: 2000
                }
            }

            B.CircularImage {
                id: ring
                anchors.fill: parent
                source: "ring.png"
                degree: engine.duration > 0 ? engine.rate*360 : 0
                rotation: -90
            }

            Image {
                id: grab
                width: 12; height: 12
                source: "grab.png"
                opacity: 0.8
                readonly property real radius: 37.5
                x: bg.c.x - 6 + (radius-0.5) * Math.sin(ring.radian)
                y: bg.c.y - 6 - (radius-0.5) * Math.cos(ring.radian)
                visible: engine.running
            }

            MouseArea {
                id: ringMouse
                anchors.fill: parent
                readonly property real gap: 4
                readonly property real r1: grab.radius - gap
                readonly property real r2: grab.radius + gap
                property alias c: bg.c

                function setRate(x, y) {
                    if (engine.duration <= 0)
                        return
                    var dx = x - c.x
                    var dy = y - c.y
                    engine.rate = (Math.PI + Math.atan2(-dx, dy))/(2*Math.PI)
                }
                hoverEnabled: true

                onPressed: {
                    var dx = mouse.x - c.x, dy = mouse.y - c.y
                    var rr = dx*dx + dy*dy
                    if (mouse.accepted = r1*r1 <= rr && rr <= r2*r2)
                        setRate(mouse.x, mouse.y)
                }
                onPositionChanged: {
                    if (pressed) setRate(mouse.x, mouse.y)
                    var dx = mouse.x - c.x, dy = mouse.y - c.y
                    var rr = dx*dx + dy*dy
                    if (mouse.accepted = r1*r1 <= rr && rr <= r2*r2) {
                        tooltipHider.stop()
                        var pos = Qt.point(mouse.x, mouse.y);
                        var time = text.formatTime(engine.time) + "/" + text.formatTime(engine.end)
                        B.App.window.showToolTip(ringMouse, pos, time)
                    } else
                        tooltipHider.start()
                }
                onExited: tooltipHider.start()
                Timer {
                    id: tooltipHider
                    repeat: false
                    interval: 1000
                    onTriggered: B.App.window.hideToolTip()
                }
            }

            B.Button {
                id: play;
                anchors.centerIn: parent; width: 40; height: 40
                readonly property string prefix: engine.playing ? "pause" : "play"
                action: "play/play-pause"; icon.prefix: prefix
                mask: prefix + ".png"
                Item {
                    readonly property real gap: engine.playing ? 4 : 2
                    width: parent.width
                    height: gap + (40-2*gap)*(1-engine.volume/100.0)
                    clip: true
                    Image {
                        width: 40; height: 40
                        source: play.prefix + "-filled.png"
                    }
                }
            }
        }
    }
}

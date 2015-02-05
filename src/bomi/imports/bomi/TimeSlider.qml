import QtQuick 2.0
import bomi 1.0

Slider {
    id: seeker
    property Component markerStyle
    property alias time: seeker.value
    min: d.engine.begin; max: d.engine.end

    Component {
        id: dummy;
        Item {
            property var marker
            property var chapter
            x: seeker.width * chapter.rate
        }
    }

    QtObject {
        id: d;
        readonly property Engine engine: App.engine
        property bool ticking: false
        property var markers: []
        property var style
        function generateChapters() {
            var i;
            for (i=0; i<markers.length; ++i)
                Util.delete_(markers[i])
            markers = []
            if (style)
                style.destroy()
            if (!markerStyle)
                return
            style = markerStyle.createObject(seeker)
            style.control = seeker
            var chapters = engine.chapters
            for (i=0; i<chapters.length; ++i) {
                var chapter = chapters[i]
                var wrap = dummy.createObject(seeker, { "chapter" : chapter })
                wrap.marker = style.marker.createObject(wrap, { "parent" : wrap })
                markers.push(wrap)
            }
        }
    }
    Connections {
        target: d.engine
        onTick: {
            d.ticking = true;
            seeker.value = d.engine.time
            d.ticking = false;
        }
        onChaptersChanged: d.generateChapters()
    }
    onValueChanged: if (!d.ticking) d.engine.seek(value)
    Component.onCompleted: { d.generateChapters() }
}

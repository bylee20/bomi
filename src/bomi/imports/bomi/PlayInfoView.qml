import QtQuick 2.0
import QtQuick.Layouts 1.0
import bomi 1.0

Item {
    id: wrapper
    property alias show: wrapper.visible
    readonly property Engine engine: App.engine
    readonly property var audio: engine.audio
    readonly property var video: engine.video
    readonly property var sub: engine.subtitle

//    onVisibleChanged: if (visible) bringIn.start()
    NumberAnimation {
        id: bringIn; target: box; properties: "scale"; running: false
        from: 0.0;     to: 1.0; easing {type: Easing.OutBack; overshoot: 1.1}
    }
//    visible: false

    Component.onCompleted: { bringIn.start() }

    ColumnLayout {
        id: box; spacing: 0
//        readonly property alias fontSize: wrapper.fontSize
        PlayInfoText { content: engine.media.name }
        PlayInfoText {
            readonly property int rate: engine.rate*1000
            content: qsTr("State: %2/%3(%4%) ×%5 [%1]").arg(engine.stateText)
                .arg(Format.time(engine.time_s*1000)).arg(Format.time(engine.end_s*1000))
                .arg(engine.time_s > 0 && engine.end_s > 0 ? (rate/10) : 0)
                .arg(engine.speed.toFixed(2))
        }
        PlayInfoText {
            readonly property int sync: engine.avSync
            content: qsTr("Audio/Video Sync: %1%2ms")
                .arg(sync < 0 ? "" : sync > 0 ? "+" : "±").arg(sync);
        }

        PlayInfoText { }

        PlayInfoText {
            property real usage: App.cpu.usage
            content: qsTr("CPU Usage: %1%(avg. %2%/core)")
                .arg(usage.toFixed(0)).arg((usage/App.cpu.cores).toFixed(1));
        }
        PlayInfoText {
            property real usage: App.memory.usage
            content: qsTr("RAM Usage: %3MiB(%4% of %5GiB)")
                .arg(usage.toFixed(1)).arg((usage/App.memory.total*100.0).toFixed(1))
                .arg((App.memory.total/1024.0).toFixed(2));
        }
        PlayInfoText {
            readonly property int used: engine.cacheUsed
            readonly property int size: engine.cacheSize
            readonly property real percent: 100.0*used/size
            content: qsTr("Cache: %1")
                .arg(!size ? qsTr("Unavailable")
                           : qsTr("%1KiB(%3% of %2KiB)").arg(used).arg(size)
                                .arg(percent.toFixed(1)))
        }

        PlayInfoText { }

        PlayInfoTrack { id: ti; name: qsTr("Video Track"); info: video }
        PlayInfoVideoOutput { format: video.input; name: qsTr("Input   ") }
        PlayInfoVideoOutput { format: video.output; name: qsTr("Output  ") }
        PlayInfoVideoOutput { format: video.renderer; name: qsTr("Renderer") }

        function activationText(s) {
            switch (s) {
            case Engine.Unavailable: return qsTr("Unavailable")
            case Engine.Deactivated: return qsTr("Deactivated")
            case Engine.Activated:   return qsTr("Activated")
            default:                 return ""
            }
        }

        PlayInfoText {
            content: qsTr("Est. Frame Number: %1/%2").arg(video.frameNumber).arg(video.frameCount);
        }
        PlayInfoText {
            content: qsTr("Dropped Frames: %1 (%2fps)").arg(video.droppedFrames).arg(video.droppedFps.toFixed(3));
        }
        PlayInfoText {
            content: qsTr("Delayed Frames: %1 (%2ms)").arg(video.delayedFrames).arg(video.delayedTime);
        }

        PlayInfoText {
            readonly property var hw: video.hwacc
            content: qsTr("Hardware Acceleration: %1[%2]")
                .arg(box.activationText(hw.state)).arg(Format.textNA(hw.driver))
        }

        PlayInfoText {
            readonly property var hw: video.hwacc
            content: qsTr("Deinterlacer: %3").arg(box.activationText(video.deinterlacer))
        }

        PlayInfoText { }

        PlayInfoTrack { name: qsTr("Audio Track"); info: audio }
        PlayInfoAudioOutput { format: audio.input; name: qsTr("Input   ") }
        PlayInfoAudioOutput { format: audio.output; name: qsTr("Output  ") }
        PlayInfoAudioOutput { format: audio.renderer;  name: qsTr("Renderer") }
        PlayInfoText {
            readonly property real gain: audio.normalizer * 100
            content: qsTr("Normalizer: %1[%2%]")
                .arg(gain < 0 ? qsTr("Deactivated") : qsTr("Activated"))
                .arg(gain < 0 ? "--" : gain.toFixed(1))
        }
        PlayInfoText {
            content: qsTr("Driver: %1[%2]")
                .arg(audio.driver.length > 0 ? audio.driver : "--")
                .arg(audio.device)
        }

        PlayInfoText { }

        Component {
            id: subtitleTrack
            PlayInfoTrack {
                name: qsTr("Subtitle Track")
            }
        }

        Repeater {
            model: engine.subtitle
            Loader {
                readonly property var data: modelData
                sourceComponent: subtitleTrack
                onItemChanged: {
                    if (item)
                        item.info = modelData
                }
            }
        }

//        PlayInfoSubtitleList { list: sub.tracks; name: qsTr("Subtitle Track") }
    }
}

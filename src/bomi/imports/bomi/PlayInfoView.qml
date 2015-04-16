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

    NumberAnimation {
        id: bringIn; target: box; properties: "scale"; running: false
        from: 0.0; to: 1.0; easing { type: Easing.OutBack; overshoot: 1.1 }
    }

    Component.onCompleted: { bringIn.start() }

    ColumnLayout {
        id: box; spacing: 0
        PlayInfoText { content: engine.media.name }
        PlayInfoText {
            readonly property real percent: Alg.trunc(engine.rate*100, 1)
            readonly property string name: qsTr("State")
            readonly property string end: Format.time(engine.end_s*1000)
            readonly property string speed: engine.speed.toFixed(2) + "x";
            content: name + ": " + Format.time(engine.time_s*1000) + '/' + end
                     + '(' + (engine.time_s > 0 && engine.end_s > 0 ? percent : 0).toFixed(1) + "%), "
                     + speed + " [" + engine.stateText + ']'
        }
        PlayInfoText {
            readonly property int sync: engine.avSync
            readonly property string sign: sync < 0 ? "" : sync > 0 ? "+" : "±"
            readonly property string name: qsTr("Audio/Video Sync")
            content: name + ": " + sign + engine.avSync + "ms"
        }

        PlayInfoText { }

        PlayInfoText {
            readonly property real usage: Alg.trunc(App.cpu.usage, 1)
            readonly property real avg: usage/App.cpu.cores
            readonly property string name: qsTr("CPU Usage")
            readonly property string suffix: qsTr("core")
            content: formatBracket(name, usage.toFixed(1) + "%", avg.toFixed(1) + '%/' + suffix)
        }
        PlayInfoText {
            readonly property real usage: Alg.trunc(App.memory.usage, 1)
            readonly property string name: qsTr("RAM Usage")
            readonly property string suffix: "%/" + (App.memory.total/1024.0).toFixed(2) + "GiB"
            content: formatBracket(name, usage.toFixed(1) + "MiB", (usage/App.memory.total*100.0).toFixed(1) + suffix)
        }
        PlayInfoText {
            readonly property int used: engine.cache.used
            readonly property int size: engine.cache.size
            readonly property real percent: size ? Alg.trunc(100.0*used/size, 1) : 0
            readonly property string suffix: "%/" + Format.integerNA(size) + "KiB"
            readonly property string name: qsTr("Cache")
            content: formatBracket(name, !size ? qsTr("Unavailable") : (used + "KiB"),
                                   Format.fixedNA(percent, 1) + suffix);
        }

        PlayInfoText { }

        PlayInfoTrack { id: ti; info: video }
        PlayInfoVideoOutput { format: video.decoder; name: qsTr("Decoder") }
        PlayInfoVideoOutput { format: video.filter;  name: qsTr("Filter ") }
        PlayInfoVideoOutput { format: video.output;  name: qsTr("Output ") }

        PlayInfoText {
            readonly property string name: qsTr("Est. Frame Number")
            readonly property string count: '/' + video.frameCount
            content: name + ": " + video.frameNumber + count
        }
        PlayInfoText {
            readonly property string name: qsTr("Dropped Frames")
            content: formatBracket(name, video.droppedFrames, video.droppedFps.toFixed(3) + "fps")
        }
        PlayInfoText {
            readonly property string name: qsTr("Delayed Frames")
            content: formatBracket(name, video.delayedFrames, video.delayedTime.toFixed(3) + "ms")
        }

        Component {
            id: toolText
            PlayInfoText {
                content: formatBracket(name, activationText(tool.state), Format.textNA(tool.driver))
            }
        }
        Loader {
            readonly property string name: qsTr("Hardware Acceleration")
            readonly property QtObject tool: video.hardwareAcceleration
            sourceComponent: toolText
        }
        Loader {
            readonly property string name: qsTr("Deinterlacer")
            readonly property QtObject tool: video.deinterlacer
            sourceComponent: toolText
        }

        PlayInfoText { }

        PlayInfoTrack { info: audio }
        PlayInfoAudioOutput { format: audio.decoder; name: qsTr("Decoder") }
        PlayInfoAudioOutput { format: audio.filter;  name: qsTr("Filter ") }
        PlayInfoAudioOutput { format: audio.output;  name: qsTr("Output ") }
        PlayInfoText {
            readonly property real gain: Alg.trunc(audio.normalizer * 100, 1)
            readonly property int state: gain < 0 ? Engine.Deactivated : Engine.Activated
            readonly property string name: qsTr("Normalizer")
            content: formatBracket(name, activationText(state), Format.fixedNA(gain, 1, 0) + "%")
        }
        PlayInfoText {
            readonly property string name: qsTr("Driver")
            content: formatBracket(name, Format.textNA(audio.driver), audio.device)
        }

        PlayInfoText { }

        Component {
            id: subtitleTrack
            PlayInfoText {
                readonly property string name: engine.subtitle.track.typeText
                function format(name, track) {
                    if (name.length <= 0) return ""
                    return qsTr("%1 #%2: Codec=%3, Title=%4, Language=%5, Encoding=%6")
                        .arg(name)
                        .arg(Format.integerNA(track.number))
                        .arg(Format.textNA(track.codec))
                        .arg(Format.textNA(track.title))
                        .arg(Format.textNA(track.language))
                        .arg(Format.textNA(track.encoding))
                }
                width: wrapper.width
                textStyle.wrapMode: Text.Wrap
                content: format(name, track)
            }
        }

        Repeater {
            model: engine.subtitle.selection
            Loader {
                readonly property QtObject track: modelData
                sourceComponent: subtitleTrack
//                onItemChanged: { if (item) item.info = modelData }
            }
        }
    }
}

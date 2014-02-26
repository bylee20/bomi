import QtQuick 2.0
import QtQuick.Layouts 1.0
import CMPlayer 1.0

Item {
	id: wrapper
	readonly property int fontSize: parent.height*0.022;
	width: parent.width-fontSize*2;
	height: parent.height-fontSize*2;
	anchors.centerIn: parent
	property string fontFamily: Util.monospace
	property alias show: wrapper.visible
	Connections {
		target: engine
		onAudioChanged: audioinfo.update()
		onVideoChanged: videoinfo.update()
	}

	Component.onCompleted: {
		audioinfo.update()
		videoinfo.update()
	}

	onVisibleChanged: if (visible) bringIn.start()
	NumberAnimation {
		id: bringIn; target: box; properties: "scale"; running: false
		from: 0.0;	 to: 1.0; easing {type: Easing.OutBack; overshoot: 1.1}
	}
	visible: false

	property real __cpu: 0.0
	property real __mem: 0.0
	property real __fps: 1.0
	property real __sync: 0.0
	property real __volnorm: 1.0

	Timer {
		running: parent.visible
		interval: 1000
		repeat: true
		onTriggered: {
			__cpu = Util.cpu
			__mem = Util.memory
			__fps = engine.avgfps
			__sync = engine.avgsync
			__volnorm = engine.volumeNormalizerActivated ? engine.volumeNormalizer*100.0 : -1.0;
		}
	}
	ColumnLayout {
		id: box; spacing: 0
		readonly property alias fontSize: wrapper.fontSize
		PlayInfoText { text: engine.media.name }
		PlayInfoText {
			readonly property int time: engine.time/1000
			readonly property int end: engine.end/1000
			readonly property int pos10: engine.relativePosition*1000
			text: "[%1]%2/%3(%4%)".arg(engine.stateText)
				.arg(Util.secToString(time)).arg(Util.secToString(end)).arg(time > 0 && end > 0 ? (pos10/10.0).toFixed(1) : 0)
		}
		PlayInfoText { text: qsTr("Playback Speed: ×%1").arg(engine.speed.toFixed(2)); }
		PlayInfoText { }

		PlayInfoText {
			property alias cpu: wrapper.__cpu
			text: qsTr("CPU Usage: %1%(avg. %2%/core)").arg(cpu.toFixed(0)).arg((cpu/Util.cores).toFixed(1));
		}
		PlayInfoText {
			property alias mem: wrapper.__mem
			text: qsTr("RAM Usage: %3MB(%4% of %5GB)")
				.arg(mem.toFixed(1)).arg((mem/Util.totalMemory*100.0).toFixed(1))
				.arg((Util.totalMemory/1024.0).toFixed(2));
		}
		PlayInfoText { text: qsTr("Cache: %1").arg(engine.cache < 0 ? qsTr("Unavailable") : (engine.cache*100.0).toFixed(0) + "%"); }
		PlayInfoText { }

		PlayInfoText { text: qsTr("Audio/Video Sync: %6ms").arg(wrapper.__sync.toFixed(1)); }
		PlayInfoText {
			property alias fps: wrapper.__fps
			text: qsTr("Avg. Frame Rate: %7fps(%8MB/s)").arg(fps.toFixed(3)).arg((engine.bitrate(fps)/(8*1024*1024)).toFixed(2));
		}
		PlayInfoText {
			text: qsTr("Dropped Frames: %1 frames").arg(engine.droppedFrames)
		}
		PlayInfoText {
			property alias volnorm: wrapper.__volnorm
			text: qsTr("Volume Normalizer: %1").arg(volnorm < 0 ? qsTr("Deactivated") : (volnorm.toFixed(1) + "%"))
		}
		PlayInfoText { }

		function number(v, n) {
			return v > 0 ? ("" + (n === undefined ? v : v.toFixed(n))) : "--";
		}

		PlayInfoText {
			id: videoinfo
			function update() {
				var txt = qsTr("Video Codec: %1").arg(engine.video.codec);
				txt += '\n';
				txt += qsTr("Input : %1 %2x%3 %4fps(%5MB/s)")
					.arg(engine.video.input.type)
					.arg(box.number(engine.video.input.size.width))
					.arg(box.number(engine.video.input.size.height))
					.arg(box.number(engine.video.input.fps, 3))
					.arg(box.number(engine.video.input.bitrate/(8*1024*1024), 2));
				txt += '\n';
				txt += qsTr("Output: %1 %2x%3 %4fps(%5MB/s)")
					.arg(engine.video.output.type)
					.arg(box.number(engine.video.output.size.width))
					.arg(box.number(engine.video.output.size.height))
					.arg(box.number(engine.video.output.fps, 3))
					.arg(box.number(engine.video.output.bitrate/(8*1024*1024), 2));
				txt += '\n';
				txt += qsTr("Hardware Acceleration: %1").arg(engine.video.hwacc);
				text = txt
			}
		}
		PlayInfoText { }

		PlayInfoText {
			id: audioinfo
			function update() {
				var txt = qsTr("Audio Codec: %1").arg(engine.audio.codec);
				txt += '\n';
				txt += qsTr("Input : %1 %2kbps %3kHz %4 %5bits")
					.arg(engine.audio.input.type)
					.arg(box.number(engine.audio.input.bitrate/1024, 0))
					.arg(box.number(engine.audio.input.samplerate))
					.arg(engine.audio.input.channels)
					.arg(box.number(engine.audio.input.bits));
				txt += '\n';
				txt += qsTr("Output: %1 %2kbps %3kHz %4 %5bits")
					.arg(engine.audio.output.type)
					.arg(box.number(engine.audio.output.bitrate/1024, 0))
					.arg(box.number(engine.audio.output.samplerate))
					.arg(engine.audio.output.channels)
					.arg(box.number(engine.audio.output.bits));
				txt += '\n';
				txt += qsTr("Output Driver: %1").arg(engine.audio.driver);
				text = txt
			}
		}
	}
}

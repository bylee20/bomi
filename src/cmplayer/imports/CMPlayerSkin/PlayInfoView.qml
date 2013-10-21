import QtQuick 2.0
import CMPlayerCore 1.0

Item {
	id: wrapper
	x: fontSize*0.5
	y: x
	width:box.width+x*2
	height:box.height+x*2
	property string fontFamily: Util.monospace
	readonly property int fontSize: parent.height*0.022;
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
	Timer {
		id: timer
		running: parent.visible
		interval: 1000
		repeat: true
		onTriggered: { timeinfo.update(); resources.update(); }
	}
	Item {
		id: box
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.topMargin: fontSize*0.5
		anchors.leftMargin: fontSize*0.5
		width: childrenRect.width
		height: childrenRect.height
		Text {
			id: medianame
			anchors.top: parent.top
			anchors.left: parent.left
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
			text: engine.media.name
		}
		Text {
			id: timeinfo
			anchors.top: medianame.bottom
			anchors.left: parent.left
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			function update() {
				text = "[%1]%2/%3(%4%)"
				.arg(engine.stateText)
				.arg(Util.msecToString(engine.time))
				.arg(Util.msecToString(engine.end))
				.arg((engine.relativePosition*100.0).toFixed(1));
			}
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
		}
		Text {
			id: speed
			anchors.top: timeinfo.bottom
			anchors.left: parent.left
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
			text: qsTr("Playback Speed: ×%1").arg(engine.speed.toFixed(2));
		}
		Text {
			id: resources
			anchors.top: speed.bottom
			anchors.left: parent.left
			anchors.topMargin: wrapper.fontSize
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			function update() {
				var cpu = Util.cpu; var mem = Util.memory; var fps = engine.avgfps;
				var txt = qsTr("CPU Usage: %1%(avg. %2%/core)")
					.arg(cpu.toFixed(0))
					.arg((cpu/Util.cores).toFixed(1));
				txt += '\n';
				txt += qsTr("RAM Usage: %3MB(%4% of %5GB)")
					.arg(mem.toFixed(1))
					.arg((mem/Util.totalMemory*100.0).toFixed(1))
					.arg((Util.totalMemory/1024.0).toFixed(2));
				txt += '\n';
				txt += qsTr("Audio/Video Sync: %6ms").arg(engine.avgsync.toFixed(1));
				txt += '\n';
				txt += qsTr("Avg. Frame Rate: %7fps(%8MB/s)")
					.arg(fps.toFixed(3)).arg((engine.bps(fps)/(8*1024*1024)).toFixed(2));
				txt += '\n';
				txt += qsTr("Volume Normalizer: %1")
					.arg(engine.volumeNormalizerActivated ? ((engine.volumeNormalizer*100.0).toFixed(1) + "%") : qsTr("Deactivated"));
				text = txt;
			}
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
		}
		Text {
			id: videoinfo
			anchors.top: resources.bottom
			anchors.left: parent.left
			anchors.topMargin: wrapper.fontSize
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			function update() {
				var txt = qsTr("Video Codec: %1").arg(engine.video.codec);
				txt += '\n';
				txt += qsTr("Input : %1 %2x%3 %4fps(%5MB/s)")
					.arg(engine.video.input.type)
					.arg(engine.video.input.size.width)
					.arg(engine.video.input.size.height)
					.arg(engine.video.input.fps.toFixed(3))
					.arg((engine.video.input.bps/(8*1024*1024)).toFixed(2));
				txt += '\n';
				txt += qsTr("Output: %1 %2x%3 %4fps(%5MB/s)")
					.arg(engine.video.output.type)
					.arg(engine.video.output.size.width)
					.arg(engine.video.output.size.height)
					.arg(engine.video.output.fps.toFixed(3))
					.arg((engine.video.output.bps/(8*1024*1024)).toFixed(2));
				text = txt
			}
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
		}
		Text {
			id: hwaccinfo
			anchors.top: videoinfo.bottom
			anchors.left: parent.left
			anchors.topMargin: 0
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			text: qsTr("Hardware Acceleration: %1").arg(engine.hardwareAccelerationText);
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
		}

		Text {
			id: audioinfo
			anchors.top: hwaccinfo.bottom
			anchors.left: parent.left
			anchors.topMargin: wrapper.fontSize
			width: contentWidth
			height: contentHeight
			font.pixelSize: wrapper.fontSize
			font.family: wrapper.fontFamily
			function update() {
				var txt = qsTr("Audio Codec: %1").arg(engine.audio.codec);
				txt += '\n';
				txt += qsTr("Input : %1 %2kbps %3kHz %4ch %5bits")
					.arg(engine.audio.input.type)
					.arg((engine.audio.input.bps*1e-3).toFixed(0))
					.arg(engine.audio.input.samplerate)
					.arg(engine.audio.input.channels)
					.arg(engine.audio.input.bits);
				txt += '\n';
				txt += qsTr("Output: %1 %2kbps %3kHz %4ch %5bits")
					.arg(engine.audio.output.type)
					.arg((engine.audio.output.bps*1e-3).toFixed(0))
					.arg(engine.audio.output.samplerate)
					.arg(engine.audio.output.channels)
					.arg(engine.audio.output.bits);
				txt += '\n';
				txt += qsTr("Output Driver: %1").arg(engine.audio.audioDriverText);
				text = txt
			}
			color: "yellow"
			style: Text.Outline
			styleColor: "black"
		}
	}
}

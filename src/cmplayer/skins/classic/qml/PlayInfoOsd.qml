import QtQuick 2.0
import CMPlayer 1.0

Item {
	id: view
	anchors.fill: parent
	visible: false
	property PlayInfo info
	property string fontFamily: ""
	property int fontSize: parent.height*0.03;
	function collectNow() {info.collect(); timeinfo.make(); resources.make();}
	property var playinfo: Item {}
	onInfoChanged: {
		fontFamily = info.monospace
		info.mediaChanged.connect(medianame.make)
		info.audioChanged.connect(audioinfo.make)
		info.videoChanged.connect(videoinfo.make)
	}
	Timer {
		id: timer
		running: (parent.visible && info)
		interval: 1000
		repeat: true
		onTriggered: if (info) parent.collectNow()
	}
	Text {
		id: medianame
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		height: contentHeight
		font.pixelSize: view.fontSize
		font.family: view.fontFamily
		color: "yellow"
		style: Text.Outline
		styleColor: "black"
		function make() {
			text = info.media.name
		}
	}
	Text {
		id: timeinfo
		anchors.top: medianame.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		height: contentHeight
		font.pixelSize: parent.fontSize
		font.family: parent.fontFamily
		function make() {
			text = "[%1]%2/%3(%4%)"
				.arg(info.stateText)
				.arg(info.msecToString(info.time))
				.arg(info.msecToString(info.duration))
				.arg((info.time/info.duration).toFixed(1));
		}
		color: "yellow"
		style: Text.Outline
		styleColor: "black"
	}
	Text {
		id: resources
		anchors.top: timeinfo.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.topMargin: parent.fontSize
		height: contentHeight
		font.pixelSize: parent.fontSize
		font.family: parent.fontFamily
		function make() {
			text = qsTr("CPU usage: %1%(avg. per core)\nRAM usage: %2MB(%3% of %4GB)\nAvg. A-V sync: %5ms\nAvg. frame rate: %6fps(%7MB/s)")
			.arg(info.cpu.toFixed(1))
			.arg(info.memory.toFixed(1))
			.arg((1e2*info.memory/info.totalMemory).toFixed(1))
			.arg((info.totalMemory/1024.0).toFixed(2))
			.arg(info.avgsync.toFixed(1))
			.arg(info.avgfps.toFixed(3))
			.arg((info.avgbps/(8*1024*1024)).toFixed(2));
		}
		color: "yellow"
		style: Text.Outline
		styleColor: "black"
	}
	Text {
		id: videoinfo
		anchors.top: resources.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.topMargin: parent.fontSize
		height: contentHeight
		font.pixelSize: parent.fontSize
		font.family: parent.fontFamily
		function make() {
			var txt = qsTr("Video Codec: %1 %2\n")
			.arg(info.video.codec).arg(info.video.isHardwareAccelerated ? qsTr("[HW acc.]") : "");
			txt += qsTr("Input : %1 %2x%3 %4fps(%5MB/s)\n")
			.arg(info.video.input.type)
			.arg(info.video.input.size.width)
			.arg(info.video.input.size.height)
			.arg(info.video.input.fps.toFixed(3))
			.arg((info.video.input.bps/(8*1024*1024)).toFixed(2));
			txt += qsTr("Output: %1 %2x%3 %4fps(%5MB/s)")
			.arg(info.video.output.type)
			.arg(info.video.output.size.width)
			.arg(info.video.output.size.height)
			.arg(info.video.output.fps.toFixed(3))
			.arg((info.video.output.bps/(8*1024*1024)).toFixed(2));
			text = txt
		}
		color: "yellow"
		style: Text.Outline
		styleColor: "black"
	}

	Text {
		id: audioinfo
		anchors.top: videoinfo.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.topMargin: parent.fontSize
		height: contentHeight
		font.pixelSize: parent.fontSize
		font.family: parent.fontFamily
		function make() {
			var txt = qsTr("Audio Codec: %1\n").arg(info.audio.codec);
			txt += qsTr("Input : %1 %2kbps %3kHz %4ch %5bits\n")
			.arg(info.audio.input.type)
			.arg((info.audio.input.bps*1e-3).toFixed(0))
			.arg(info.audio.input.samplerate)
			.arg(info.audio.input.channels)
			.arg(info.audio.input.bits);
			txt += qsTr("Output: %1 %2kbps %3kHz %4ch %5bits")
			.arg(info.audio.output.type)
			.arg((info.audio.output.bps*1e-3).toFixed(0))
			.arg(info.audio.output.samplerate)
			.arg(info.audio.output.channels)
			.arg(info.audio.output.bits);
			text = txt
		}
		color: "yellow"
		style: Text.Outline
		styleColor: "black"
	}
}

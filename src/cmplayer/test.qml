import QtQuick 2.0
import CMPlayer 1.0
import "qml"

Rectangle {
	id: topItem
	VideoRenderer {
		id: video
		property string name: "video"
		anchors.fill: parent
		SubtitleRenderer {
			anchors.fill: parent
		}

		TextOsd {
			id: message
			text: "test"
			name: "message"
			anchors.fill: parent
		}
		ProgressOsd {
			anchors.fill: parent
			name: "timeline"

		}
		PlayInfo {
			id: playinfo
			anchors.fill: parent
			name: "playinfo"
			visible: false
			property string fontFamily: monospace
			property int fontSize: parent.height*0.03;
			function collectNow() {collect(); timeinfo.make(); resources.make();}
			Timer {
				id: timer
				running: playinfo.visible
				interval: 1000
				repeat: true
				onTriggered: playinfo.collectNow()
			}
			Text {
				id: medianame
				anchors.top: parent.top
				anchors.left: parent.left
				anchors.right: parent.right
				height: contentHeight
				font.pixelSize: playinfo.fontSize
				font.family: playinfo.fontFamily
				color: "yellow"
				style: Text.Outline
				styleColor: "black"
			}
			onMediaChanged: {medianame.text = media.name;}
			Text {
				id: timeinfo
				anchors.top: medianame.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				height: contentHeight
				font.pixelSize: playinfo.fontSize
				font.family: playinfo.fontFamily
				function make() {
					text = "[%1]%2/%3(%4%)"
						.arg(playinfo.stateText)
						.arg(playinfo.msecToString(playinfo.time))
						.arg(playinfo.msecToString(playinfo.duration))
						.arg((playinfo.time/playinfo.duration).toFixed(1));
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
				anchors.topMargin: playinfo.fontSize
				height: contentHeight
				font.pixelSize: playinfo.fontSize
				font.family: playinfo.fontFamily
				function make() {
					text = qsTr("CPU usage: %1%(avg. per core)\nRAM usage: %2MB(%3% of %4GB)\nAvg. A-V sync: %5ms\nAvg. frame rate: %6fps(%7MB/s)")
					.arg(playinfo.cpu.toFixed(1))
					.arg(playinfo.memory.toFixed(1))
					.arg((1e2*playinfo.memory/playinfo.totalMemory).toFixed(1))
					.arg((playinfo.totalMemory/1024.0).toFixed(2))
					.arg(playinfo.avgsync.toFixed(1))
					.arg(playinfo.avgfps.toFixed(3))
					.arg((playinfo.avgbps/(8*1024*1024)).toFixed(2));
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
				anchors.topMargin: playinfo.fontSize
				height: contentHeight
				font.pixelSize: playinfo.fontSize
				font.family: playinfo.fontFamily
				function make() {
					var txt = qsTr("Video Codec: %1 %2\n")
					.arg(playinfo.video.codec).arg(playinfo.video.isHardwareAccelerated ? qsTr("[HW acc.]") : "");
					txt += qsTr("Input : %1 %2x%3 %4fps(%5MB/s)\n")
					.arg(playinfo.video.input.type)
					.arg(playinfo.video.input.size.width)
					.arg(playinfo.video.input.size.height)
					.arg(playinfo.video.input.fps.toFixed(3))
					.arg((playinfo.video.input.bps/(8*1024*1024)).toFixed(2));
					txt += qsTr("Output: %1 %2x%3 %4fps(%5MB/s)")
					.arg(playinfo.video.output.type)
					.arg(playinfo.video.output.size.width)
					.arg(playinfo.video.output.size.height)
					.arg(playinfo.video.output.fps.toFixed(3))
					.arg((playinfo.video.output.bps/(8*1024*1024)).toFixed(2));
					text = txt
				}
				color: "yellow"
				style: Text.Outline
				styleColor: "black"
			}
			onVideoChanged: videoinfo.make()
			Text {
				id: audioinfo
				anchors.top: videoinfo.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.topMargin: playinfo.fontSize
				height: contentHeight
				font.pixelSize: playinfo.fontSize
				font.family: playinfo.fontFamily
				function make() {
					var txt = qsTr("Audio Codec: %1\n").arg(playinfo.audio.codec);
					txt += qsTr("Input : %1 %2kbps %3kHz %4ch %5bits\n")
					.arg(playinfo.audio.input.type)
					.arg((playinfo.audio.input.bps*1e-3).toFixed(0))
					.arg(playinfo.audio.input.samplerate)
					.arg(playinfo.audio.input.channels)
					.arg(playinfo.audio.input.bits);
					txt += qsTr("Output: %1 %2kbps %3kHz %4ch %5bits")
					.arg(playinfo.audio.output.type)
					.arg((playinfo.audio.output.bps*1e-3).toFixed(0))
					.arg(playinfo.audio.output.samplerate)
					.arg(playinfo.audio.output.channels)
					.arg(playinfo.audio.output.bits);
					text = txt
				}
				color: "yellow"
				style: Text.Outline
				styleColor: "black"
			}
			onAudioChanged: audioinfo.make()
		}
	}
}

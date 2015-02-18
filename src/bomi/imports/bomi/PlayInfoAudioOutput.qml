import QtQuick 2.0
import bomi 1.0 as B

PlayInfoText {
    property var format
    property string name
    content: "%1: %2[%3bits] %4Hz %5 %6kbps"
        .arg(name).arg(format.type).arg(B.Format.integerNA(format.depth))
        .arg(B.Format.integerNA(format.samplerate))
        .arg(format.channels).arg(B.Format.fixedNA(format.bitrate/1e3, 1))
}

import QtQuick 2.0

PlayInfoText {
    property var format
    property string name
    text: "%1: %2[%3bits] %4Hz %5 %6kbps"
        .arg(name).arg(format.type).arg(formatNumber(format.depth))
        .arg(formatNumber(format.samplerate, 0))
        .arg(format.channels).arg(formatNumber(format.bitrate/1e3, 0))
}

import QtQuick 2.0

PlayInfoText {
    property var format
    property string name
    text: "%1: %2[%3bits] %4 %5bpp %6fps %7Mbps %8[%9]"
    .arg(name)
    .arg(format.type)
    .arg(formatNumber(format.depth))
    .arg(formatSize(format.size))
    .arg(formatNumber(format.bpp))
    .arg(formatNumber(format.fps, 3))
    .arg(formatNumber(format.bitrate/1e6, 1))
    .arg(format.space).arg(format.range)
}

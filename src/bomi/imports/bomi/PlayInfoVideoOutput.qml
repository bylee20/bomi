import QtQuick 2.0
import bomi 1.0 as B

PlayInfoText {
    property var format
    property string name
    content: "%1: %2[%3bits] %4 %5bpp %6fps %7Mbps %8[%9]"
        .arg(name)
        .arg(format.type)
        .arg(B.Format.integerNA(format.depth))
        .arg(B.Format.sizeNA(format.size))
        .arg(B.Format.integerNA(format.bpp))
        .arg(B.Format.fixedNA(format.fps, 3))
        .arg(B.Format.fixedNA(format.bitrate/1e6, 1))
        .arg(format.space).arg(format.range)
}

import QtQuick 2.0
import bomi 1.0 as B

PlayInfoText {
    property var format
    property string name
    readonly property string mbps: {
        if (format.bitrate < 1e6)
            return B.Format.fixedNA(format.bitrate, 0, "bps")
        return B.Format.fixedNA(format.bitrate*1e-6, 1, "Mbps")
    }
    readonly property string kbps: {
        if (format.bitrate < 1e3)
            return B.Format.fixedNA(format.bitrate, 0, "bps")
        return B.Format.fixedNA(format.bitrate*1e-3, 1, "kbps")
    }
    readonly property string prefix: name + ": " + format.type + '['
                                     + B.Format.integerNA(format.depth, "bits]");
}

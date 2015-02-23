import QtQuick 2.0
import bomi 1.0 as B

PlayInfoAvOutput {
    content: prefix + ' ' + B.Format.integerNA(format.samplerate, "Hz")
             + ' ' + format.channels + ' ' + kbps
}

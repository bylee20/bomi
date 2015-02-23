import QtQuick 2.0
import bomi 1.0 as B

PlayInfoAvOutput {
    content: prefix + ' ' + B.Format.sizeNA(format.size) + ' '
             + B.Format.integerNA(format.bpp) + "bpp "
             + B.Format.fixedNA(format.fps, 3) + "fps "
             + mbps + ' ' + format.space + '[' + format.range + ']'
}

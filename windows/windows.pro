TEMPLATE = aux

INSTALLER = bomi-setup

BIN_DIR = C:/Qt/ifw/bin

INPUT = $$PWD/packages
make.input = INPUT
make.output = $$PWD/$$INSTALLER
make.commands = $$BIN_DIR/binarycreator -v -n \
    -c $$PWD/config/config.xml \
    -p $$PWD/packages ${QMAKE_FILE_OUT}
make.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += make

DISTFILES += \
    config/config.xml \
    config/controlscript.js \
    packages/net.xylosper.bomi.prog/meta/package.xml \
    packages/net.xylosper.bomi.prog/meta/installscript.js \
    packages/net.xylosper.bomi.prog/meta/targetwidget.ui \
    packages/net.xylosper.bomi.prog.i386/meta/package.xml \
    packages/net.xylosper.bomi.prog.x86_64/meta/package.xml \
    packages/net.xylosper.bomi.prog.post/meta/package.xml \
    packages/net.xylosper.bomi.prog.post/meta/installscript.js \
    packages/net.xylosper.bomi.ytdl/meta/package.xml \

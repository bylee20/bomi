QT       += core gui

TARGET = cmplayer_skin
TEMPLATE = lib
CONFIG += debug_and_release designer plugin release

DESTDIR = ../../build/lib
!isEmpty(DO_INSTALL) {
	target.path = $$[QT_INSTALL_PLUGINS]/designer
	INSTALLS += target
} else {
	CONFIG += static
}

HEADERS += \
	widgets.hpp

SOURCES += \
	widgets.cpp


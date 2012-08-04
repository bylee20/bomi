QT       += core gui

TARGET = cmplayer_widgets
TEMPLATE = lib
CONFIG += debug_and_release designer plugin release

!isEmpty(TEST) {
DESTDIR = ../../build/lib
CONFIG += static
} else {
DESTDIR = $$[QT_INSTALL_PLUGINS]/designer
}

HEADERS += \
    widgets.hpp

SOURCES += \
    widgets.cpp


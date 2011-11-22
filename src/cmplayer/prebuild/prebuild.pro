TEMPLATE = app
TARGET = prebuild
macx:CONFIG-=app_bundle

QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.6.sdk
CONFIG += debug_and_release release
QT = core

HEADERS += \
    enumgenerator.hpp \
    enumtemplate.hpp

SOURCES += \
    main.cpp \
    enumgenerator.cpp \
    enumtemplate.cpp

OTHER_FILES += \
    enum-list

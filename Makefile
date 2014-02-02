LIBQUVI_SUFFIX ?= $(shell if `pkg-config --exists libquvi-0.9`; then echo "-0.9"; fi)
CXX ?= g++

kern := $(shell uname -s)
os := $(shell if test $(kern) = "Darwin"; then echo "osx"; elif test $(kern) = "Linux"; then echo "linux"; else echo "unknown"; fi)
qmake_vars := RELEASE=\\\"yes\\\" LIBQUVI_SUFFIX=$(LIBQUVI_SUFFIX)
install_file := install -m 644
install_exe := install -m 755
install_dir := sh install_dir.sh
pkg_config_path := $(shell pwd)/build/lib/pkgconfig:${PKG_CONFIG_PATH}

ifeq ($(os),osx)
	QT_PATH = /usr/local/opt/qt5
	QMAKE ?= $(QT_PATH)/bin/qmake -spec macx-clang
	MACDEPLOYQT ?= $(QT_PATH)/bin/macdeployqt
	cmplayer_exec := CMPlayer
	cmplayer_exec_path := build/$(cmplayer_exec).app/Contents/MacOS
	njobs = $(shell sysctl hw.ncpu | awk '{print $$2}')
else
	PREFIX ?= /usr/local
	QMAKE ?= qmake-qt5
	BIN_PATH ?= $(PREFIX)/bin
	DATA_PATH ?= $(PREFIX)/share
	ICON_PATH ?= $(DATA_PATH)/icons/hicolor
	APP_PATH ?= $(DATA_PATH)/applications
	ACTION_PATH ?= $(DATA_PATH)/apps/solid/actions
	CMPLAYER_SKINS_PATH ?= $(DATA_PATH)/cmplayer/skins
	CMPLAYER_IMPORTS_PATH ?= $(DATA_PATH)/cmplayer/imports
	cmplayer_exec := cmplayer
	qmake_vars := $(qmake_vars) \
		DEFINES+="CMPLAYER_SKINS_PATH=\\\\\\\"$(CMPLAYER_SKINS_PATH)\\\\\\\"" \
		DEFINES+="CMPLAYER_IMPORTS_PATH=\\\\\\\"$(CMPLAYER_IMPORTS_PATH)\\\\\\\"" \
		QMAKE_CXX=$(CXX)
	njobs = $(shell nproc)
endif

cmplayer: skins imports
	cd src/cmplayer && PKG_CONFIG_PATH=$(pkg_config_path) $(QMAKE) $(qmake_vars) cmplayer.pro && $(MAKE) -j$(njobs) release
ifeq ($(os),osx)
	cp -r build/skins $(cmplayer_exec_path)
	cp -r build/imports $(cmplayer_exec_path)
endif

cmplayer-bundle: cmplayer
	cp -r $(QT_PATH)/qml/QtQuick.2 $(cmplayer_exec_path)/imports
	install -d $(cmplayer_exec_path)/imports/QtQuick
	cp -r $(QT_PATH)/qml/QtQuick/Controls $(cmplayer_exec_path)/imports/QtQuick
	cp -r $(QT_PATH)/qml/QtQuick/Layouts $(cmplayer_exec_path)/imports/QtQuick
	cp -r /usr/local/Cellar/libquvi/0.4.1/libquvi-scripts/share/libquvi-scripts/lua $(cmplayer_exec_path)
#	rm `find $(cmplayer_exec_path) -name '*_debug.dylib'`
#	cd build && $(MACDEPLOYQT) $(cmplayer_exec).app
#	./fix-dep
	cd build && $(MACDEPLOYQT) $(cmplayer_exec).app -dmg

skins: build_dir
	cp -r src/cmplayer/skins build

imports: build_dir
	cp -r src/cmplayer/imports build
	
build_dir:
	install -d build

clean:
	-cd src/cmplayer && PKG_CONFIG_PATH=$(pkg_config_path) $(QMAKE) $(qmake_vars) cmplayer.pro && make clean && rm -rf Makefile* debug release
	-rm -rf build/CMPlayer*
	-rm -rf build/cmplayer*
	-rm -rf build/skins
	-rm -rf build/imports

install: cmplayer
ifeq ($(os),linux)
	-install -d $(DEST_DIR)$(BIN_PATH)
	-install -d $(DEST_DIR)$(APP_PATH)
	-install -d $(DEST_DIR)$(ACTION_PATH)
	-install -d $(DEST_DIR)$(ICON_PATH)/16x16/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/22x22/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/24x24/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/32x32/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/48x48/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/64x64/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/128x128/apps
	-install -d $(DEST_DIR)$(ICON_PATH)/256x256/apps
	-install -d $(DEST_DIR)$(CMPLAYER_SKINS_PATH)
	-install -d $(DEST_DIR)$(CMPLAYER_IMPORTS_PATH)
#	-install -d $(DEST_DIR)$(ICON_PATH)/scalable/apps
	$(install_exe) build/$(cmplayer_exec) $(DEST_DIR)$(BIN_PATH)
	$(install_file) cmplayer.desktop $(DEST_DIR)$(APP_PATH)
	$(install_file) cmplayer-opendvd.desktop $(DEST_DIR)$(ACTION_PATH)
	$(install_file) icons/cmplayer16.png $(DEST_DIR)$(ICON_PATH)/16x16/apps/cmplayer.png
	$(install_file) icons/cmplayer22.png $(DEST_DIR)$(ICON_PATH)/22x22/apps/cmplayer.png
	$(install_file) icons/cmplayer24.png $(DEST_DIR)$(ICON_PATH)/24x24/apps/cmplayer.png
	$(install_file) icons/cmplayer32.png $(DEST_DIR)$(ICON_PATH)/32x32/apps/cmplayer.png
	$(install_file) icons/cmplayer48.png $(DEST_DIR)$(ICON_PATH)/48x48/apps/cmplayer.png
	$(install_file) icons/cmplayer64.png $(DEST_DIR)$(ICON_PATH)/64x64/apps/cmplayer.png
	$(install_file) icons/cmplayer128.png $(DEST_DIR)$(ICON_PATH)/128x128/apps/cmplayer.png
	$(install_file) icons/cmplayer256.png $(DEST_DIR)$(ICON_PATH)/256x256/apps/cmplayer.png
#	$(install_file) icons/cmplayer.svg $(DEST_DIR)$(ICON_PATH)/scalable/apps/cmplayer.svg
	-cp -r build/skins/* $(DEST_DIR)$(CMPLAYER_SKINS_PATH)/
	-cp -r build/imports/* $(DEST_DIR)$(CMPLAYER_IMPORTS_PATH)/
endif

# DYLD_FALLBACK_LIBRARY_PATH /Applications/VLC.app/Contents/MacOS/lib

kern := $(shell uname -s)
os := $(shell if test $(kern) = "Darwin"; then echo "osx"; elif test $(kern) = "Linux"; then echo "linux"; else echo "unknown"; fi)
qmake_vars := DESTDIR=\\\"../../bin\\\" RELEASE=\\\"yes\\\"
vlc_plugins_dir := vlc-plugins
install_file := install -m 644
install_exe := install -m 755
install_dir := sh install_dir.sh

ifeq ($(os),osx)
	QMAKE ?= /Developer/Tools/Qt/qmake -spec macx-g++
	MACDEPLOYQT ?= /Developer/Tools/Qt/macdeployqt
	LRELEASE ?= /Developer/Tools/Qt/lrelease
	cmplayer_exec := CMPlayer
	cmplayer_exec_path := build/$(cmplayer_exec).app/Contents/MacOS
else
	PREFIX ?= /usr/local
	QMAKE ?= qmake
	LRELEASE ?= lrelease
	BIN_PATH ?= $(PREFIX)/bin
	DATA_PATH ?= $(PREFIX)/share
	ICON_PATH ?= $(DATA_PATH)/icons/hicolor
	APP_PATH ?= $(DATA_PATH)/applications
	ACTION_PATH ?= $(DATA_PATH)/apps/solid/actions
	CMPLAYER_SKIN_PATH ?= $(DATA_PATH)/cmplayer/skin
	cmplayer_exec := cmplayer
	qmake_vars := $(qmake_vars) \
		DEFINES+="CMPLAYER_SKINS_PATH=\\\\\\\"$(CMPLAYER_SKINS_PATH)\\\\\\\""
endif

cmplayer: translations skin
	cd src/cmplayer && $(QMAKE) $(qmake_vars) cmplayer.pro && make release
ifeq ($(os),osx)
	cp -r build/skins $(cmplayer_exec_path)
	cd build && macdeployqt $(cmplayer_exec).app -dmg
endif

translations:
	cd src/cmplayer/translations && $(LRELEASE) cmplayer_ko.ts -qm cmplayer_ko.qm
	cd src/cmplayer/translations && $(LRELEASE) cmplayer_en.ts -qm cmplayer_en.qm

skin: build_dir
	cd src/cmplayer_skin && $(QMAKE) cmplayer_skin.pro && make release
	cp -r src/cmplayer_skin/skins build
#	$(install_dir) src/skin bin/skin

build_dir:
	install -d build

clean:
	-cd src/cmplayer && $(QMAKE) $(qmake_vars) cmplayer.pro && make clean
	-rm -rf build/CMPlayer*

install: cmplayerstatic
ifeq ($(os),linux)
	-install -d $(DEST_DIR)$(BIN_PATH)
	-install -d $(DEST_DIR)$(CMPLAYER_VLC_PLUGINS_PATH)
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
#	-install -d $(DEST_DIR)$(ICON_PATH)/scalable/apps
	$(install_exe) bin/$(cmplayer_exec) $(DEST_DIR)$(BIN_PATH)
	$(install_file) bin/$(vlc_plugins_dir)/libcmplayer*_plugin.so $(DEST_DIR)$(CMPLAYER_VLC_PLUGINS_PATH) 
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
endif

uninstall:
ifeq ($(os),linux)
	-rm -f $(BIN_PATH)/cmplayer
	-rm -f $(CMPLAYER_VLC_PLUGINS_PATH)/libcmplayer*_plugin.so
	-rm -f $(APP_PATH)/cmplayer.desktop
	-rm -f $(ACTION_PATH)/cmplayer-opendvd.desktop
	-rm -f $(ICON_PATH)/16x16/apps/cmplayer.png
	-rm -f $(ICON_PATH)/22x22/apps/cmplayer.png
	-rm -f $(ICON_PATH)/24x24/apps/cmplayer.png
	-rm -f $(ICON_PATH)/32x32/apps/cmplayer.png
	-rm -f $(ICON_PATH)/48x48/apps/cmplayer.png
	-rm -f $(ICON_PATH)/64x64/apps/cmplayer.png
	-rm -f $(ICON_PATH)/128x128/apps/cmplayer.png
	-rm -f $(ICON_PATH)/256x256/apps/cmplayer.png
	-rmdir $(BIN_PATH)
	-rmdir $(CMPLAYER_VLC_PLUGINS_PATH)
	-rmdir $(APP_PATH)
	-rmdir $(ACTION_PATH)
	-rmdir $(ICON_PATH)/16x16/apps
	-rmdir $(ICON_PATH)/22x22/apps
	-rmdir $(ICON_PATH)/24x24/apps
	-rmdir $(ICON_PATH)/32x32/apps
	-rmdir $(ICON_PATH)/48x48/apps
	-rmdir $(ICON_PATH)/64x64/apps
	-rmdir $(ICON_PATH)/128x128/apps
	-rmdir $(ICON_PATH)/256x256/apps
endif

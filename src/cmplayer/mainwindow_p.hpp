#ifndef MAINWINDOW_P_HPP
#define MAINWINDOW_P_HPP

#include "app.hpp"
#include "avmisc.hpp"
#include "timelineosdrenderer.hpp"
#include "playinfoview.hpp"
#include "colorproperty.hpp"
#include "playlistview.hpp"
#include "historyview.hpp"
#include "playlistmodel.hpp"
#include "subtitlerenderer.hpp"
#include "charsetdetector.hpp"
#include "snapshotdialog.hpp"
#include "subtitle_parser.hpp"
#include "audiocontroller.hpp"
#include "recentinfo.hpp"
#include "abrepeater.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "translator.hpp"
#include "videorenderer.hpp"
#include "appstate.hpp"
#include "pref.hpp"
#include "playlist.hpp"
#include "dialogs.hpp"
#include "toolbox.hpp"
#include "rootmenu.hpp"
#include "info.hpp"
#include <QtGui/QMouseEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <qmath.h>
#include "skin.hpp"
#include "prefdialog.hpp"

quint32 HWACCEL_FORMAT = 0;

struct MainWindow::Data {
	RootMenu &menu = RootMenu::get();
	RecentInfo &recent = RecentInfo::get();
	const Pref &p = Pref::get();

	PlayEngine &engine = PlayEngine::get();
	AudioController audio;
	VideoRenderer &video = engine.renderer();

	SubtitleRenderer subtitle;
	TimeLineOsdRenderer timeLine;
	TextOsdRenderer message = {Qt::AlignTop | Qt::AlignLeft};

	QPoint prevPos;		QTimer hider;
	bool moving = false, changingSub = false;
	bool pausedByHiding = false, dontShowMsg = false, dontPause = false;
	bool stateChanging = false;
	ABRepeater ab = {&engine, &subtitle};
	PlayInfoView playInfo = {&engine, &audio, &video};
	VideoScreen &screen = video.screen();
	Skin skin;
	QMenu *context;
	PlaylistView *playlist;
	HistoryView *history;
	QRect screenRect;
//	FavoritesView *favorite;
#ifndef Q_WS_MAC
	QSystemTrayIcon *tray;
#endif
	QString filePath;
// methods
	bool checkAndPlay(const Mrl &mrl) {
		if (mrl != engine.mrl())
			return false;
		if (!engine.isPlaying())
			engine.play();
		return true;
	}

	void openWith(const Pref::OpenMedia &mode, const Mrl &mrl) {
		auto d = this;
		if (d->checkAndPlay(mrl))
			return;
		Playlist playlist;
		if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
			playlist.append(mrl);
		} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist) {
			d->playlist->clear();
			playlist.append(mrl);
		} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist) {
			d->playlist->clear();
			playlist = PlaylistView::generatePlaylist(mrl);
		} else
			return;
		d->playlist->merge(playlist);
		d->engine.setMrl(mrl, mode.start_playback);
		if (!mrl.isDvd())
			RecentInfo::get().stack(mrl);
	}

	void sync_subtitle_file_menu() {
		if (changingSub)
			return;
		changingSub = true;
		Menu &list = menu("subtitle")("list");
		ActionGroup *g = list.g();
		const QList<SubtitleRenderer::Loaded> loaded = subtitle.loaded();
		while (g->actions().size() < loaded.size()) {
			list.addActionToGroupWithoutKey("", true);
		}
		while (g->actions().size() > loaded.size()) {
			delete g->actions().last();
		}
		const QList<QAction*> actions = g->actions();
		Q_ASSERT(loaded.size() == actions.size());
		for (int i=0; i<actions.size(); ++i) {
			actions[i]->setText(loaded[i].name());
			actions[i]->setData(i);
			actions[i]->setChecked(loaded[i].isSelected());
		}
		changingSub = false;
	}

	void load_state() {
		dontShowMsg = true;

		const AppState &as = AppState::get();

		engine.setSpeed(as.play_speed);

		menu("video")("aspect").g()->trigger(as.video_aspect_ratio);
		menu("video")("crop").g()->trigger(as.video_crop_ratio);
		menu("video")("align").g("horizontal")->trigger(as.video_alignment.id() & 0x0f);
		menu("video")("align").g("vertical")->trigger(as.video_alignment.id() & 0xf0);
		video.setOffset(as.video_offset);
		video.setEffects((VideoRenderer::Effects)as.video_effects);
		for (int i=0; i<16; ++i) {
			if ((as.video_effects >> i) & 1)
				menu("video")("filter").g()->setChecked(1 << i, true);
		}
		video.setColorProperty(as.video_color);

		audio.setVolume(as.audio_volume);
		audio.setMuted(as.audio_muted);
		audio.setPreAmp(as.audio_amp);
		audio.setVolumeNormalized(as.audio_volume_normalized);

		menu("subtitle").g("display")->trigger((int)as.sub_letterbox);
		menu("subtitle").g("align")->trigger((int)as.sub_align_top);
		subtitle.setPos(as.sub_pos);
		subtitle.setDelay(as.sub_sync_delay);

		menu("tool")["auto-exit"]->setChecked(as.auto_exit);

		menu("window").g("sot")->trigger(as.screen_stays_on_top.id());

		dontShowMsg = false;
	}

	void save_state() const {
		AppState &as = AppState::get();
		as.video_aspect_ratio = video.aspectRatio();
		as.video_crop_ratio = video.cropRatio();
		as.video_alignment = video.alignment();
		as.video_offset = video.offset();
		as.video_effects = video.effects();
		as.video_color = video.colorProperty();
		as.audio_volume = audio.volume();
		as.audio_volume_normalized = audio.isVolumeNormalized();
		as.audio_muted = audio.isMuted();
		as.audio_amp = audio.preAmp();
		as.play_speed = engine.speed();
		as.sub_pos = subtitle.pos();
		as.sub_sync_delay = subtitle.delay();
		as.screen_stays_on_top = stay_on_top_mode();
		as.sub_letterbox = subtitle.osd().letterboxHint();
		as.sub_align_top = subtitle.isTopAligned();
		as.save();
	}

	Enum::StaysOnTop stay_on_top_mode() const {
		const int id = menu("window").g("sot")->checkedAction()->data().toInt();
		return Enum::StaysOnTop::from(id, Enum::StaysOnTop::Playing);
	}

	void apply_pref() {
		SubtitleParser::setMsPerCharactor(p.ms_per_char);
		Translator::load(p.locale);
		subtitle.osd().setStyle(p.sub_style);
		menu.update();
		menu.save();
	#ifndef Q_WS_MAC
		tray->setVisible(p.enable_system_tray);
	#endif
	}
};

#endif // MAINWINDOW_P_HPP

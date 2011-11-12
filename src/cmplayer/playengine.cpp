#include "audiocontroller.hpp"
//#include "videoscene.hpp"
#include "playengine.hpp"
#include "recentinfo.hpp"
#include "vlcmedia.hpp"
#include "libvlc.hpp"
#include "pref.hpp"
#include "mrl.hpp"
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtCore/QVariant>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

struct PlayEngine::Data {
	int stoppedTime, duration, prevTick;
	bool seekable, hasVideo;
	double speed;
	libvlc_media_player_t *mp;
	VLCMedia *media;
	MediaState state;
	MediaStatus status;
	QTimer ticker;
};

PlayEngine::PlayEngine(libvlc_media_player_t *mp): d(new Data) {
	qRegisterMetaType<MediaState>("MediaState");
	d->mp = mp;
	d->media = 0;
	d->prevTick = 0;
	d->stoppedTime = -1;
	d->state = StoppedState;
	d->status = NoMediaStatus;
	d->hasVideo = d->seekable = false;
	d->speed = 1.0;
	d->duration = 0;

	connect(this, SIGNAL(_ticking()), this, SLOT(ticking()));
	connect(this, SIGNAL(_updateDuration(int)), this, SLOT(updateDuration(int)));
	connect(this, SIGNAL(_updateSeekable(bool)), this, SLOT(updateSeekable(bool)));
	connect(this, SIGNAL(_updateState(MediaState)), this, SLOT(updateState(MediaState)));
}

PlayEngine::~PlayEngine() {
	d->ticker.stop();
	delete d->media;
	delete d;
}

void PlayEngine::updateDuration(int duration) {
	if (duration > 0)
		emit durationChanged(d->duration = duration);
}

void PlayEngine::updateSeekable(bool seekable) {
	if (d->seekable != seekable)
		emit seekableChanged(d->seekable = seekable);
}

void PlayEngine::parseEvent(const libvlc_event_t *event) {
	Q_ASSERT(d->mp != 0);
	switch (event->type) {
	case libvlc_MediaPlayerSeekableChanged: {
		const bool seekable = libvlc_media_player_is_seekable(d->mp);
		emit _updateSeekable(seekable);
		break;
	} case libvlc_MediaPlayerTimeChanged:
		emit _ticking();
		break;
	case libvlc_MediaPlayerPlaying:
		emit _updateState(PlayingState);
		break;
	case libvlc_MediaPlayerPaused:
		emit _updateState(PausedState);
		break;
	case libvlc_MediaPlayerEndReached:
		emit _updateState(FinishedState);
		break;
	case libvlc_MediaPlayerOpening:
	case libvlc_MediaPlayerBuffering:
	case libvlc_MediaPlayerStopped:
	case libvlc_MediaPlayerEncounteredError:
		emit _updateState(StoppedState);
		break;
	case libvlc_MediaPlayerLengthChanged:
		emit _updateDuration(event->u.media_player_length_changed.new_length);
		break;
	case libvlc_MediaPlayerTitleChanged:
		break;
	default:
		break;
	}
}

void PlayEngine::setMrl(const Mrl &mrl) {
	if (isPlaying())
		stop();
	d->duration = 0;
	emit durationChanged(d->duration);
	if (d->media) {
		if (d->media->mrl() != mrl) {
			delete d->media;
			d->media = 0;
		}
	}
	if (!d->media) {
		d->media = new VLCMedia(mrl);
		emit mrlChanged(d->media->mrl());
	}
}

bool PlayEngine::atEnd() const {
	return d->status == EosStatus;
}

MediaState PlayEngine::state() const {
	return d->state;
}

bool PlayEngine::isSeekable() const {
	return d->seekable;
}

PlayEngine::TrackList PlayEngine::parseTrackDesc(libvlc_track_description_t *desc) {
	if (!desc)
		return TrackList();
	TrackList tracks;
	while (desc) {
		Track track;
//		track.id = desc->i_id;
		track.name = QString::fromLocal8Bit(desc->psz_name);
		tracks << track;
		desc = desc->p_next;
	}
	libvlc_track_description_release(desc);
	return tracks;
}

void PlayEngine::updateState(MediaState state) {
	Q_ASSERT(d->mp != 0);
	if (d->state != state) {
		const MediaState old = d->state;
		d->state = state;
//		qDebug() << libvlc_media_player_get_length(d->mp);
//		qDebug() << libvlc_media_player_get_time(d->mp);
//		qDebug() << libvlc_media_player_get_position(d->mp);
		if (d->state == PlayingState && old != PausedState) {

			//
		}
		emit stateChanged(d->state, old);
		if (d->state == FinishedState) {
			emit aboutToFinished();
			if (d->media)
				emit finished(d->media->mrl());
		} else if (d->state == StoppedState) {
			if (d->stoppedTime >= 0 && d->media) {
				emit stopped(d->media->mrl(), d->stoppedTime, duration());
				d->stoppedTime = -1;
			}
		}
	}
}

double PlayEngine::speed() const {
	return d->speed;
}

void PlayEngine::setSpeed(double speed) {
	Q_ASSERT(d->mp != 0);
	if (qFuzzyCompare(speed, 1.0))
		speed = 1.0;
	if (!qFuzzyCompare(d->speed, speed)) {
		d->speed = speed;
		if (isPlaying() || isPaused())
			libvlc_media_player_set_rate(d->mp, d->speed);
		emit speedChanged(d->speed);
	}
}

void PlayEngine::setStatus(MediaStatus status) {
	if (d->status != status) {
		emit statusChanged(d->status = status);
	}
}

bool PlayEngine::play() {
	Q_ASSERT(d->mp != 0);
	if (isPaused()) {
		libvlc_media_player_set_pause(d->mp, 0);
		updateState(PlayingState);
		return true;
	}
	if (!d->media)
		return false;

	int seek = -1;
	const RecentInfo &recent = RecentInfo::get();
	const int record = recent.stoppedTime(d->media->mrl());
	if (record > 0) {
		if (Pref::get().ask_record_found) {
			const QDateTime date = recent.stoppedDate(d->media->mrl());
			const QString title = tr("Stopped Record Found");
			const QString text = tr("This file was stopped during its playing before.\n"
				"Played Date: %1\nStopped Time: %2\n"
				"Do you want to start from where it's stopped?\n"
				"(You can configure not to ask anymore in the preferecences.)")
				.arg(date.toString(Qt::ISODate)).arg(msecToString(record, "h:mm:ss"));
			const QMessageBox::StandardButtons b = QMessageBox::Yes | QMessageBox::No;
			if (QMessageBox::question(QApplication::activeWindow(), title, text, b) == QMessageBox::Yes)
				seek = record;
		} else
			seek = record;
	}
	if (seek > 0)
		d->media->addOption("start-time", (double)seek/1000.0);
	libvlc_media_player_set_media(d->mp, d->media->media());
	if (libvlc_media_player_play(d->mp) != 0) {
	    qDebug() << "libvlc exception:" << libvlc_errmsg();
	    return false;
	}
	return true;
}

bool PlayEngine::pause() {
	Q_ASSERT(d->mp != 0);
	libvlc_media_player_set_pause(d->mp, 1);
	updateState(PausedState);
	return true;
}

void PlayEngine::stop() {
	Q_ASSERT(d->mp != 0);
	if (isPlaying() || isPaused())
		d->stoppedTime = position();
	else
		d->stoppedTime = -1;
	libvlc_media_player_stop(d->mp);
	updateState(StoppedState);
}

bool PlayEngine::seek(int ms) {
	Q_ASSERT(d->mp != 0);
	if (d->state == PlayingState || d->state == PausedState) {
		libvlc_media_player_set_time(d->mp, ms);
		ticking();
		return true;
	} else
		return false;
}

int PlayEngine::duration() const {
	return d->duration;
}

int PlayEngine::position() const {
	Q_ASSERT(d->mp != 0);
	int ret = 0;
	if (isPlaying() || isPaused())
		ret = libvlc_media_player_get_time(d->mp);
	return ret;
}

void PlayEngine::ticking() {
	int tick = position();
	if (d->prevTick != tick) {
		emit this->tick(d->prevTick = tick);
	}
}

Mrl PlayEngine::mrl() const {
	return d->media ? d->media->mrl() : Mrl();
}

bool PlayEngine::hasVideo() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_media_player_has_vout(d->mp) > 0;
}

int PlayEngine::videoTrackCount() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_video_get_track_count(d->mp);
}

int PlayEngine::audioTrackCount() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_audio_get_track_count(d->mp);
}

int PlayEngine::spuCount() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_video_get_spu_count(d->mp);
}

int PlayEngine::titleCount() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_media_player_get_title_count(d->mp);
}

int PlayEngine::chapterCount() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_media_player_get_chapter_count(d->mp);
}

int PlayEngine::currentVideoTrackId() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_video_get_track(d->mp);
}

int PlayEngine::currentAudioTrackId() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_audio_get_track(d->mp);
}

int PlayEngine::currentTitleId() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_media_player_get_title(d->mp);
}

int PlayEngine::currentChapterId() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_media_player_get_chapter(d->mp);
}

int PlayEngine::currentSPUId() const {
	Q_ASSERT(d->mp != 0);
	return libvlc_video_get_spu(d->mp);
}

PlayEngine::TrackList PlayEngine::audioTracks() const {
	Q_ASSERT(d->mp != 0);
	if (audioTrackCount() > 0)
		return parseTrackDesc(libvlc_audio_get_track_description(d->mp));
	return TrackList();
}

PlayEngine::TrackList PlayEngine::videoTracks() const {
	Q_ASSERT(d->mp != 0);
	if (videoTrackCount() > 0)
		return parseTrackDesc(libvlc_video_get_track_description(d->mp));
	return TrackList();
}

PlayEngine::TrackList PlayEngine::chapters() const {
	Q_ASSERT(d->mp != 0);
	if (chapterCount() <= 0)
		return TrackList();
	return parseTrackDesc(libvlc_video_get_chapter_description(d->mp, currentTitleId()));
}

PlayEngine::TrackList PlayEngine::titles() const {
	Q_ASSERT(d->mp != 0);
	if (titleCount() > 0)
		return parseTrackDesc(libvlc_video_get_title_description(d->mp));
	return TrackList();
}

PlayEngine::TrackList PlayEngine::spus() const {
	Q_ASSERT(d->mp != 0);
	if (spuCount() > 0)
		return parseTrackDesc(libvlc_video_get_spu_description(d->mp));
	return TrackList();
}

void PlayEngine::setCurrentAudioTrack(int id) {
	Q_ASSERT(d->mp != 0);
	libvlc_audio_set_track(d->mp, id);
}

void PlayEngine::setCurrentSPU(int id) {
	Q_ASSERT(d->mp != 0);
	libvlc_video_set_spu(d->mp, id);
}

void PlayEngine::setCurrentTitle(int id) {
	Q_ASSERT(d->mp != 0);
	libvlc_media_player_set_title(d->mp, id);
}

void PlayEngine::setCurrentChapter(int id) {
	Q_ASSERT(d->mp != 0);
	libvlc_media_player_set_chapter(d->mp, id);
}

void PlayEngine::setCurrentVideoTrack(int id) {
	Q_ASSERT(d->mp != 0);
	libvlc_video_set_track(d->mp, id);
}

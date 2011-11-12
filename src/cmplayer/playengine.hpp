#ifndef PLAYENGINE_HPP
#define PLAYENGINE_HPP

#include <QtCore/QObject>
#include "global.hpp"
#include "mrl.hpp"
#include <vlc/vlc.h>

class NativeVideoRenderer;	class AudioController;
class VideoScene;

class PlayEngine : public QObject {
	Q_OBJECT
public:
	struct Track {QString name;};
	typedef QList<Track> TrackList;
	~PlayEngine();
	int position() const;
	void setMrl(const Mrl &mrl);
	MediaState state() const;
	bool isSeekable() const;
	void setSpeed(double speed);
	double speed() const;
	bool hasVideo() const;
	MediaStatus status() const;
	int duration() const;
	bool atEnd() const;
	Mrl mrl() const;
	bool isPlaying() const {return state() == PlayingState;}
	bool isPaused() const {return state() == PausedState;}
	bool isStopped() const {return state() == StoppedState;}
	int currentVideoTrackId() const;
	int currentAudioTrackId() const;
	int currentTitleId() const;
	int currentChapterId() const;
	int currentSPUId() const;
	TrackList audioTracks() const;
	TrackList videoTracks() const;
	TrackList chapters() const;
	TrackList titles() const;
	TrackList spus() const;
	void setCurrentAudioTrack(int id);
	void setCurrentTitle(int id);
	void setCurrentChapter(int id);
	void setCurrentSPU(int id);
	void setCurrentVideoTrack(int id);
	int videoTrackCount() const;
	int audioTrackCount() const;
	int spuCount() const;
	int titleCount() const;
	int chapterCount() const;
//	static PlayEngine &get() {Q_ASSERT(obj != 0); return *obj;}
public slots:
	bool play();
	void stop();
	bool pause();
	bool seek(int pos);
signals:
	void aboutToFinished();
	void stopped(Mrl mrl, int pos, int duration);
	void finished(Mrl mrl);
	void tick(int pos);
	void mrlChanged(const Mrl &mrl);
	void stateChanged(MediaState state, MediaState old);
	void seekableChanged(bool seekable);
	void speedChanged(double speed);
	void positionChanged(int pos);
	void durationChanged(int duration);
	void tagsChanged();
	void statusChanged(MediaStatus status);
// internal signals
	void _updateDuration(int duration);
	void _ticking();
	void _updateSeekable(bool seekable);
	void _updateState(MediaState state);
private slots:
	void ticking();
	void updateSeekable(bool seekable);
	void updateDuration(int duration);
	void updateState(MediaState state);
private:
	void setStatus(MediaStatus status);
private:
	void updateChapterInfo();
	typedef libvlc_track_description_t TrackDesc;
	static TrackList parseTrackDesc(TrackDesc *desc);
//	void setMediaPlayer(libvlc_media_player_t *mp);
	friend class LibVLC;
	PlayEngine(libvlc_media_player_t *mp);
	void parseEvent(const libvlc_event_t *event);
	struct Data;
	Data *d;
};

#endif // PLAYENGINE_HPP

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QtGui/QMainWindow>
#include <QtGui/QSystemTrayIcon>
#include "global.hpp"

class Mrl;		class PlayEngine;
class Track;		class ControlWidget;
class VideoFormat;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();
	static MainWindow &get() {Q_ASSERT(obj != 0); return *obj;}
//	PlayEngine *engine() const;
public slots:
	void openMrl(const Mrl &mrl, const QString &enc);
	void openMrl(const Mrl &mrl);
signals:
	void fullscreenChanged(bool full);
private slots:
	void maximize();
	void onScreenSizeChanged(const QSize &size);
	void applyPref();
	void updateVideoFormat(const VideoFormat &format);
	void setEffect(QAction *action);
	void setSubtitleAlign(int data);
	void setSubtitleDisplay(int data);
//	void setFilter(QAction *action);
	void seekToSubtitle(int key);
	void about();
	void setVolumeNormalized(bool norm);
	void setTempoScaled(bool scaled);
	void openFile();
	void openDvd();
	void openLocation(const QString &loc);
	void openUrl();
	void togglePlayPause();
	void showContextMenu(const QPoint &pos);
	void updateMrl(const Mrl &mrl);
	void seek(int diff);
	void setVolume(int volume);
	void setMuted(bool muted);
	void setVideoSize(double times);
	void updateState(MediaState state, MediaState old);
	void setColorProperty(QAction *action);
	void setSpeed(int speed);
	void setAmp(int amp);
	void doRepeat(int key);
	void moveSubtitle(int dy);
	void clearSubtitles();
	void updateSubtitle(QAction *action);
	void setSyncDelay(int diff);
	void updateRecentActions(const QList<Mrl> &list);
	void setPref();
	void hideCursor();
	void handleTray(QSystemTrayIcon::ActivationReason reason);
	void openSubFile();

	void checkSubtitleMenu();
	void checkSPUMenu();
	void setSPU(QAction *act);

	void checkPlayMenu();
	void checkChapterMenu();
	void checkTitleMenu();
	void setTitle(QAction *act);
	void setChapter(QAction *act);

	void checkAudioMenu();
	void checkAudioTrackMenu();
	void setAudioTrack(QAction *act);

	void checkVideoMenu();
	void checkVideoTrackMenu();
	void setVideoTrack(QAction *act);

	void updateStaysOnTop();
	void takeSnapshot();
private:
//	ControlWidget *createControlWidget();
//	QWidget *createCentralWidget(QWidget *video, QWidget *control);
	void appendSubFiles(const QStringList &files, bool checked, const QString &enc);
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void setFullScreen(bool full);
	void resizeEvent(QResizeEvent *event);
	template<typename M, typename A>
	static typename A::mapped_type getTriggerAction(uint mod, const M &map
			, const A &act, const typename A::mapped_type &def) {
		typename M::const_iterator it = map.begin();
		for (; it != map.end(); ++it) {
			if (it.key().value() == mod && it.value().first)
				return act[it.value().second.value()];
		}
		return def;
	}
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void dropEvent(QDropEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void showMessage(const QString &message, int last = 2500);
	void showMessage(const QString &cmd, int value
		, const QString &unit, bool sign = false, int last = 2500);
	void showMessage(const QString &cmd, const QString &description, int last = 2500);
	void showMessage(const QString &cmd, double value
		, const QString &unit, bool sign = false, int last = 2500);
	void showMessage(const QString &cmd, bool value, int last = 2500);
	struct Data;
	Data *d;

	static MainWindow *obj;
};

#endif // MAINWINDOW_HPP

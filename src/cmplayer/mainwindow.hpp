#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "globalqmlobject.hpp"

class Mrl;		class PrefDialog;
class MainWindow;

class MainWindow : public QQuickView {
	Q_OBJECT
public:
	MainWindow(QWindow *parent = nullptr);
	MainWindow(const MainWindow &) = delete;
	MainWindow &operator = (const MainWindow &) = delete;
	~MainWindow();
	void openFromFileManager(const Mrl &mrl);
	bool isFullScreen() const;
public slots:
	void openMrl(const Mrl &mrl, const QString &enc);
	void openMrl(const Mrl &mrl);
	void exit();
private slots:
	void applyPref();
	void updateMrl(const Mrl &mrl);
	void setVideoSize(double rate);
	void clearSubtitles();
	void updateRecentActions(const QList<Mrl> &list);
//	void hideCursor();
	void updateStaysOnTop();
	void reloadSkin();
	void checkWindowState();
private:
	void updateTitle();
	void setCursorVisible(bool visible);
	void doVisibleAction(bool visible);
	void showMessage(const QString &message);
	void showMessage(const QString &cmd, int value, const QString &unit, bool sign = false) {showMessage(cmd, toString(value, sign) + unit);}
	void showMessage(const QString &cmd, double value, const QString &unit, bool sign = false) {showMessage(cmd, toString(value, sign) + unit);}
	void showMessage(const QString &cmd, const QString &desc) {showMessage(cmd + ": " + desc);}
	void showMessage(const QString &cmd, bool value) {showMessage(cmd, value ? tr("On") : tr("Off"));}
	void appendSubFiles(const QStringList &files, bool checked, const QString &enc);
	void setFullScreen(bool full);
	void changeEvent(QEvent *event);
	void closeEvent(QCloseEvent *event);
	void customEvent(QEvent *event);
	int getStartTime(const Mrl &mrl);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void exposeEvent(QExposeEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void dropEvent(QDropEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void resizeEvent(QResizeEvent *event);
	void moveEvent(QMoveEvent *event);
	bool event(QEvent *event);
	friend class MainView;
	struct Data;
	Data *d;
};

#endif // MAINWINDOW_HPP

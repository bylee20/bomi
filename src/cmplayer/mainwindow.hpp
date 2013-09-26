#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "globalqmlobject.hpp"

class Mrl;			class PrefDialog;
class MainView;		class Playlist;
class Subtitle;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	class Cmd;
	MainWindow(QWidget *parent = nullptr);
	MainWindow(const MainWindow &) = delete;
	MainWindow &operator = (const MainWindow &) = delete;
	~MainWindow();
	void openFromFileManager(const Mrl &mrl);
	bool isFullScreen() const;
public slots:
	void openMrl(const Mrl &mrl);
	void openMrl(const Mrl &mrl, const QString &enc);
	void exit();
private slots:
	void applyPref();
	void updateMrl(const Mrl &mrl);
	void setVideoSize(double rate);
	void clearSubtitleFiles();
	void updateRecentActions(const QList<Mrl> &list);
	void updateStaysOnTop();
	void reloadSkin();
	void checkWindowState();
private:
	// init functions
	MainView *createView();
	void connectMenus();
	void connectObjects();
	void restoreAppState();
	void initContextMenu();
	void initPlayEngine();
	void initVideoRenderer();
	void initUndoStack();
	void initTimers();

	void updateTitle();
	Playlist generatePlaylist(const Mrl &mrl) const;
	bool load(Subtitle &subtitle, const QString &fileName, const QString &encoding);

	void setCursorVisible(bool visible);
	void doVisibleAction(bool visible);
	void showMessage(const QString &message, const bool *force = nullptr);
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
	int getCache(const Mrl &mrl);
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
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
	friend class MainView;
	struct Data;
	Data *d;
	class VolumeCmd;	class SpeedCmd;		class AspectRatioCmd;
	class CropCmd;
};

template<typename Func, typename T>
class ValueCmd : public QUndoCommand {
public:
	ValueCmd(const T &to, const T &from, const Func &func): to(to), from(from), func(func) { }
	void redo() { func(to); }
	void undo() { func(from); }
private:
	T to, from; Func  func;
};

class MainView : public QQuickView {
	Q_OBJECT
public:
	MainView(MainWindow *main): QQuickView(main->windowHandle()), m_main(main) {
		setColor(Qt::black);
		setResizeMode(QQuickView::SizeRootObjectToView);
	}
private:
	void mouseDoubleClickEvent(QMouseEvent *event) {
//		UtilObject::resetFilter(UtilObject::MouseDoubleClick);
		UtilObject::resetTriggered(UtilObject::MouseDoubleClick);
		QQuickView::mouseDoubleClickEvent(event);
		if (UtilObject::isTriggered(UtilObject::MouseDoubleClick))
			m_main->mouseDoubleClickEvent(event);
	}
	void mousePressEvent(QMouseEvent *event) {
		UtilObject::resetTriggered(UtilObject::MousePress);
		QQuickView::mousePressEvent(event);
		if (UtilObject::isTriggered(UtilObject::MousePress))
			m_main->mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) {
//		UtilObject::resetFilter(UtilObject::MouseRelease);
		QQuickView::mouseReleaseEvent(event);
//		if (!UtilObject::isFiltered(UtilObject::MouseRelease))
			m_main->mouseReleaseEvent(event);
	}
	void mouseMoveEvent(QMouseEvent *event) {
//		UtilObject::resetFilter(UtilObject::MouseMove);
		QQuickView::mouseMoveEvent(event);
//		if (!UtilObject::isFiltered(UtilObject::MouseMove))
			m_main->mouseMoveEvent(event);
	}
	void keyPressEvent(QKeyEvent *event) {
//		UtilObject::resetFilter(UtilObject::KeyPress);
//		QQuickView::keyPressEvent(event);
//		if (!UtilObject::isFiltered(UtilObject::KeyPress))
			m_main->keyPressEvent(event);
	}
	void wheelEvent(QWheelEvent *event) {
		UtilObject::resetTriggered(UtilObject::Wheel);
		QQuickView::wheelEvent(event);
		if (UtilObject::isTriggered(UtilObject::Wheel))
			m_main->wheelEvent(event);
	}
	bool event(QEvent *event) {
		if (QQuickView::event(event))
			return true;
		if (event->type() == QEvent::DragMove) {
			m_main->dragEnterEvent(static_cast<QDragEnterEvent*>(event));
		} else if (event->type() == QEvent::Drop) {
			m_main->dropEvent(static_cast<QDropEvent*>(event));
		} else
			return false;
		return true;
	}
	MainWindow *m_main = nullptr;
};


#endif // MAINWINDOW_HPP

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "globalqmlobject.hpp"

class Mrl;		class PrefDialog;
class MainWindow;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	MainWindow(const MainWindow &) = delete;
	MainWindow &operator = (const MainWindow &) = delete;
	~MainWindow();
	void openFromFileManager(const Mrl &mrl);
	bool isFullScreen() const {return windowState() & Qt::WindowFullScreen;}
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
private:
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
	void keyPressEvent(QKeyEvent *event);
	void onKeyPressed(QKeyEvent *event);
	void onMouseEvent(QMouseEvent *event);
	void onWheelEvent(QWheelEvent *event);
	void dropEvent(QDropEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void resizeEvent(QResizeEvent *event);
	friend class MainView;
	struct Data;
	Data *d;
};

class MainView : public QQuickView {
public:
	MainView(MainWindow *parent): QQuickView(parent->window()->windowHandle()), p(parent) {}
private:
	void keyPressEvent(QKeyEvent *event) {
		QQuickView::keyPressEvent(event);
		p->onKeyPressed(event);
	}
	void mouseMoveEvent(QMouseEvent *event) {
		QQuickView::mouseMoveEvent(event);
		p->onMouseEvent(event);
	}
	void mouseDoubleClickEvent(QMouseEvent *event) {
		UtilObject::resetDoubleClickFilter();
		QQuickView::mouseDoubleClickEvent(event);
		p->onMouseEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) {
		QQuickView::mouseReleaseEvent(event);
		p->onMouseEvent(event);
	}
	void mousePressEvent(QMouseEvent *event) {
		QQuickView::mousePressEvent(event);
		p->onMouseEvent(event);
	}
	void wheelEvent(QWheelEvent *event) {
		QQuickView::wheelEvent(event);
		p->onWheelEvent(event);
	}
	MainWindow *p = nullptr;
};

#endif // MAINWINDOW_HPP

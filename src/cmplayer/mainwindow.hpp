#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "globalqmlobject.hpp"

class Mrl;			class PrefDialog;
class MainWindow;	class Playlist;
class Subtitle;

class MainWindow : public QWidget {
	Q_OBJECT
public:
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
//	void hideCursor();
	void updateStaysOnTop();
	void reloadSkin();
	void checkWindowState();
private:
	Playlist generatePlaylist(const Mrl &mrl) const;
	bool load(Subtitle &subtitle, const QString &fileName, const QString &encoding);
	void connectMenus();
	void updateTitle();
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
		UtilObject::resetDoubleClickFilter();
		QQuickView::mouseDoubleClickEvent(event);
		if (!UtilObject::isDoubleClickFiltered())
			m_main->mouseDoubleClickEvent(event);
	}
	void mousePressEvent(QMouseEvent *event) {
		QQuickView::mousePressEvent(event);
		m_main->mousePressEvent(event);
	}
	void mouseReleaseEvent(QMouseEvent *event) {
		QQuickView::mouseReleaseEvent(event);
		m_main->mouseReleaseEvent(event);
	}
	void mouseMoveEvent(QMouseEvent *event) {
		QQuickView::mouseMoveEvent(event);
		m_main->mouseMoveEvent(event);
	}
	void keyPressEvent(QKeyEvent *event) {
		QQuickView::keyPressEvent(event);
		m_main->keyPressEvent(event);
	}
	void wheelEvent(QWheelEvent *event) {
		QQuickView::wheelEvent(event);
		m_main->wheelEvent(event);
	}

	MainWindow *m_main = nullptr;
};


#endif // MAINWINDOW_HPP

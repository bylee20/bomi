#ifndef APP_HPP
#define APP_HPP

#include "stdafx.hpp"

class QUrl;		class Mrl;
class MainWindow;	class QMenuBar;

class App : public QApplication {
	Q_OBJECT
public:
	App(int &argc, char **argv);
	~App();
	static QIcon defaultIcon();
	void setWindowTitle(QWidget *w, const QString &title);
	void setMainWindow(MainWindow *mw);
	MainWindow *mainWindow() const;
	QStringList devices() const;
	QString styleName() const;
	bool isUnique() const;
	QStringList availableStyleNames() const;
#ifdef Q_OS_MAC
	QMenuBar *globalMenuBar() const;
#endif
	void setStyleName(const QString &name);
	void setLocale(const QLocale &locale);
	QLocale locale() const;
	void setAlwaysOnTop(QWidget *widget, bool onTop);
	void setScreensaverDisabled(bool disabled);
	void setUnique(bool unique);
	bool shutdown();
	void runCommands();
	bool isOpenGLDebugLoggerRequested() const;
	void setMprisActivated(bool activated);
public slots:
	bool sendMessage(const QString &message, int timeout = 5000);
signals:
	void messageReceived(const QString &message);
private:
	static constexpr int ReopenEvent = QEvent::User + 1;
	App(const App&) = delete;
	App &operator = (const App&) = delete;
	bool event(QEvent *event);
	struct Data;
	Data *d = nullptr;
};

#define cApp (*static_cast<App*>(qApp))

class LocalConnection : public QObject {
	Q_OBJECT
public:
	LocalConnection(const QString &id, QObject *parent = 0);
	~LocalConnection();
	bool runServer();
	bool sendMessage(const QString &message, int timeout);
signals:
	void messageReceived(const QString &message);
private:
	struct Data;
	Data *d;
};

#endif // APPLICATION_HPP

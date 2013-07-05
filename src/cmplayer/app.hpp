#ifndef APP_HPP
#define APP_HPP

#include "stdafx.hpp"

class QUrl;		class Mrl;
class MainWindow;	class QMenuBar;

struct Argument {
	QString name, value;
	static Argument fromCommand(const QString &cmd) {
		Argument arg;
		const int eq = cmd.indexOf('=');
		if (eq < 0)
			arg.name = cmd;
		else {
			arg.name = cmd.left(eq);
			arg.value = cmd.mid(eq+1);
		}
		return arg;
	}
};
typedef QList<Argument> Arguments;

class App : public QApplication {
	Q_OBJECT
public:
	App(int &argc, char **argv);
	~App();
	static QIcon defaultIcon();
	static Mrl getMrlFromCommandLine();
	static Arguments parse(const QStringList &cmds);
	void setFileName(const QString &fileName);
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
	void setAlwaysOnTop(QWindow *window, bool onTop);
	void setScreensaverDisabled(bool disabled);
	void setUnique(bool unique);
	bool shutdown();
public slots:
	bool sendMessage(const QString &message, int timeout = 5000);
signals:
	void messageReceived(const QString &message);
private slots:
	void open(const QString &url);
	void onMessageReceived(const QString &message);
private:
	static constexpr int ReopenEvent = QEvent::User + 1;
	App(const App&) = delete;
	App &operator = (const App&) = delete;
	static void messageHandler(QtMsgType type, const char *msg);
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

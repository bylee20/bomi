#ifndef APP_HPP
#define APP_HPP

#include "stdafx.hpp"
#include "qtsingleapplication/qtsingleapplication.h"

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

class App : public QtSolution::QtSingleApplication {
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
    QWindow *topWindow() const;
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

#endif // APPLICATION_HPP

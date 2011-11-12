#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "qtsingleapplication/qtsingleapplication.h"

class QUrl;		class Mrl;
class MainWindow;	class QMenuBar;

class Application : public QtSolution::QtSingleApplication {
	Q_OBJECT
public:
	Application(int &argc, char **argv);
	~Application();
	static QIcon defaultIcon();
	void setStyleName(const QString &name);
	static Mrl getMrlFromCommandLine();
	QStringList devices() const;
	void setAlwaysOnTop(QWidget *widget, bool onTop);
	void setScreensaverDisabled(bool disabled);
	void setUnique(bool unique);
	QString styleName() const;
	bool isUnique() const;
	QStringList availableStyleNames() const;
	QString test();
#ifdef Q_WS_MAC
	QMenuBar *globalMenuBar() const;
#endif
	void getProcInfo();
signals:
	void gotProcInfo(double cpu, int rss, double mem);
private slots:
	void readProcInfo();
	void initialize();
	void open(const QString &url);
	void parseMessage(const QString &message);
private:
	void loadStyle();
	static void messageHandler(QtMsgType type, const char *msg);
	bool event(QEvent *event);
	struct Data;
	Data *d;
};

static inline Application *app() {return static_cast<Application*>(qApp);}

#endif // APPLICATION_HPP

#ifndef APP_HPP
#define APP_HPP

#include "qtsingleapplication/qtsingleapplication.h"

class QUrl;		class Mrl;
class MainWindow;	class QMenuBar;

class App : public QtSolution::QtSingleApplication {
	Q_OBJECT
public:
	App(int &argc, char **argv);
	~App();
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

static inline App *app() {return static_cast<App*>(qApp);}

#endif // APPLICATION_HPP

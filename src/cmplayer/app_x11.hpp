#ifndef APP_X11_HPP
#define APP_X11_HPP

#include "stdafx.hpp"

//#ifdef Q_OS_LINUX

class AppX11 : public QObject {
	Q_OBJECT
public:
	AppX11(QObject *parent = 0);
	~AppX11();
	void setScreensaverDisabled(bool disabled);
	void setAlwaysOnTop(WId wid, bool onTop);
	QStringList devices() const;
	bool shutdown();
private slots:
	void ss_reset();
private:
	AppX11(const AppX11&) = delete;
	AppX11 &operator = (const AppX11&) = delete;
	struct Data;
	Data *d = nullptr;
};

//#endif

#endif // APPLICATION_X11_HPP

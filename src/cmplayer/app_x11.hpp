#ifndef APP_X11_HPP
#define APP_X11_HPP

#include "stdafx.hpp"

class AppX11 : public QObject {
	Q_OBJECT
public:
	AppX11(QObject *parent = 0);
	~AppX11();
	void setScreensaverDisabled(bool disabled);
	void setAlwaysOnTop(QWindow *window, bool onTop);
	QStringList devices() const;
	bool shutdown();
	void setWmName(const QString &name);
private:
	AppX11(const AppX11&) = delete;
	AppX11 &operator = (const AppX11&) = delete;
	struct Data;
	Data *d = nullptr;
};

#endif // APPLICATION_X11_HPP

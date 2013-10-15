#ifndef TRAYICON_HPP
#define TRAYICON_HPP

#include "stdafx.hpp"

class TrayIcon : public QObject {
	Q_OBJECT
public:
	enum ActivationReason {
		Quit = -1,
		Trigger = QSystemTrayIcon::Trigger,
		Context = QSystemTrayIcon::Context
	};

	TrayIcon(const QIcon &icon, QObject *parent = nullptr);
	~TrayIcon();
	void setVisible(bool visible);
signals:
	void activated(ActivationReason reason);
private:
	static void onQuit(void *menu, void *arg);
	static void onShow(void *menu, void *arg);
	struct Data;
	Data *d;
};

#endif // TRAYICON_HPP

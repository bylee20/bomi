#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

#include "stdafx.hpp"

class GlobalQmlObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString monospace READ monospace CONSTANT)
	Q_PROPERTY(bool doubleClicked READ isDoubleClicked WRITE setDoubleClicked)
public:
	GlobalQmlObject(QObject *parent = nullptr);
	Q_INVOKABLE double textWidth(const QString &text, int size) const;
	Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family) const;
	QString monospace() const;
	Q_INVOKABLE QString msecToString(int ms) {return Pch::__null_time.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
	static bool isDoubleClicked() {return m_dbc;}
	static void setDoubleClicked(bool dbc) {m_dbc = dbc;}
private:
	QFont m_font;
	static bool m_dbc;
};

#endif // GLOBALQMLOBJECT_HPP

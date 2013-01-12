#ifndef SKIN_HPP
#define SKIN_HPP

#include "stdafx.hpp"

class Skin {
public:
	~Skin() {}
	static QStringList dirs() {return data()->dirs;}
	static QStringList names(bool reload = false);
	static QFileInfo source(const QString &name) {
		auto it = data()->skins.find(name);
		if (it != data()->skins.end())
			return it.value();
		return QFileInfo();
	}
protected:
	Skin() {}
	static void plug(const QObject *sender, const char *signal, const QObject *receiver, const char *member, Qt::ConnectionType type = Qt::AutoConnection) {
		data()->connections << QObject::connect(sender, signal, receiver, member, type);
	}
	static void plug(const QObject *sender, const QMetaMethod &signal, const QObject *receiver, const QMetaMethod &method, Qt::ConnectionType type = Qt::AutoConnection) {
		data()->connections << QObject::connect(sender, signal, receiver, method, type);
	}
	template <typename Func1, typename Func2>
	static void plug(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal
			, const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot, Qt::ConnectionType type = Qt::AutoConnection) {
		data()->connections << QObject::connect(sender, signal, receiver, slot, type);
	}
	template <typename Func1, typename Func2>
	static typename QtPrivate::QEnableIf<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::Type
	plug(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot) {
		data()->connections << QObject::connect(sender, signal, slot); return data()->connections.last();
	}
private:
	struct Data {
		Data();
		QList<QMetaObject::Connection> connections;
		QStringList dirs;
		QMap<QString, QFileInfo> skins;
	};
	static Data *data() { static Data data;	return &data; }
	void unplug() {
		for (auto &connection : data()->connections)
			QObject::disconnect(connection);
		data()->connections.clear();
	}

};

#endif // SKIN_HPP

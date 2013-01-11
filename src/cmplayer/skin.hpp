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
		Data() {
#ifdef CMPLAYER_SKINS_PATH
			dirs << QString::fromLocal8Bit(CMPLAYER_SKINS_PATH);
#endif
			dirs << QDir::homePath() % _L("/.cmplayer/skins");
			dirs << QCoreApplication::applicationDirPath().toLocal8Bit() % _L("/skins");
			const QByteArray path = qgetenv("CMPLAYER_SKINS_PATH");
			if (!path.isEmpty())
				dirs << QString::fromLocal8Bit(path.data(), path.size());
		}
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

//#include "global.hpp"

//class QWidget;		class QPoint;
//class PlayEngine;	class AudioController;
//class VideoRenderer;class Mrl;

//enum Placeholder : int;

//class Skin : public QObject {
//	Q_OBJECT
//public:
//	Skin(QObject *parent = nullptr);
//	~Skin();
//	bool load(const QString &name, QWidget *parent = nullptr);
//	QWidget *widget() const;
//	QWidget *screen() const;
//	void setVisible(bool visible);
//	void connectTo(PlayEngine *engine, AudioController *audio, VideoRenderer *video);
//	QString name() const;
//	void initializePlaceholders();
//	static QStringList dirs();
//	static QStringList names(bool reload = false);
//	static QString path(const QString &name);
//	static QString mediaStateText(State state);
//public slots:
//	void setMediaNumber(int number);
//	void setTotalMediaCount(int count);
//signals:
//	void windowTitleChanged(const QString &title);
//	void windowFilePathChanged(const QString &filePath);
//private slots:
//	void onDurationChanged(int duration);
//	void onTick(int tick);
//	void seek(int time);
//	void onSeekableChanged(bool seekable);
//	void onVolumeChanged(int volume);
//	void setVolume(int volume);
//	void onActionChanged();
//	void onMrlChanged(const Mrl &mrl);
//	void onStateChanged(State state);
//private:
//	bool eventFilter(QObject *o, QEvent *e);
//	void checkChildren(QWidget *p);
//	bool checkButton(QObject *obj);
//	bool checkLabel(QObject *obj);
//	bool checkVolumeSlider(QObject *obj);
//	bool checkSeekSlider(QObject *obj);
//	void setPlaceholder(Placeholder ph, const QString &text);
//	static QWidget *topParentWidget(QWidget *widget);
//	struct Data;
//	Data *d;
//};

#undef d

#endif // SKIN_HPP

#ifndef MEDIAMISC_HPP
#define MEDIAMISC_HPP

#include "stdafx.hpp"
#include "mrl.hpp"

class PlayEngine;

class MetaData {
public:
	QString title() const { return m_title; }
	QString artist() const { return m_artist; }
	QString album() const { return m_album; }
	QString genre() const { return m_genre; }
	QString date() const { return m_date; }
	Mrl mrl() const { return m_mrl; }
	int duration() const { return m_duration; }
private:
	friend class PlayEngine;
	QString m_title, m_artist, m_album, m_genre, m_date;
	Mrl m_mrl;
	int m_duration = 0;
};

class AvIoFormat : public QObject {
	Q_OBJECT
	Q_PROPERTY(QSize size READ size CONSTANT FINAL)
	Q_PROPERTY(QString type READ type CONSTANT FINAL)
	Q_PROPERTY(int bitrate READ bitrate NOTIFY bitrateChanged)
	Q_PROPERTY(double fps READ fps CONSTANT FINAL)
	Q_PROPERTY(int bits READ bits CONSTANT FINAL)
	Q_PROPERTY(double samplerate READ samplerate CONSTANT FINAL)
	Q_PROPERTY(QString channels READ channels CONSTANT FINAL)
public:
	AvIoFormat(QObject *parent = nullptr): QObject(parent) {}
	QSize size() const {return m_size;}
	double samplerate() const {return m_samplerate;}
	int bits() const {return m_bits;}
	QString channels() const {return m_channels;}
	QString type() const {return m_type;}
	int bitrate() const {return m_bitrate;}
	double fps() const {return m_fps;}
signals:
	void bitrateChanged();
private:
	void setBps(int bitrate) { if (_Change(m_bitrate, bitrate)) emit bitrateChanged(); }
	friend class AvInfoObject;
	friend class PlayEngine;
	QSize m_size;
	QString m_type;
	int m_bitrate = 0;
	double m_fps = 0.0;
	double &m_samplerate = m_fps;
	int &m_bits = m_size.rwidth();
	QString m_channels;
};

class AvInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString codec READ codecDescription CONSTANT FINAL)
	Q_PROPERTY(AvIoFormat *input READ input CONSTANT FINAL)
	Q_PROPERTY(AvIoFormat *output READ output CONSTANT FINAL)
	Q_PROPERTY(QString driver READ driver CONSTANT FINAL)
	Q_PROPERTY(QString hwacc READ driver CONSTANT FINAL)
public:
	AvInfoObject(QObject *parent = nullptr);
	QString codecDescription() const {return m_codecDescription;}
	QString codec() const { return m_codec; }
	AvIoFormat *input() const {return m_input;}
	AvIoFormat *output() const {return m_output;}
	QString driver() const { return m_driver; }
private:
	void setHwAcc(int acc);
	AvIoFormat *m_input = new AvIoFormat(this);
	AvIoFormat *m_output = new AvIoFormat(this);
	QString m_codecDescription, m_hwacc, &m_driver = m_hwacc, m_codec;
	friend class PlayEngine;
};

class MediaInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
	MediaInfoObject(QObject *parent = nullptr): QObject(parent) {}
	QString name() const {return m_name;}
	void setName(const QString &name) { if (_Change(m_name, name)) emit nameChanged(m_name); }
signals:
	void nameChanged(const QString &name);
private:
	QString m_name;
};

struct Stream {
	enum Type {Audio = 0, Video, Subtitle, Unknown};
	QString name() const {
		QString name = m_title;
		if (!m_lang.isEmpty())
			name += name.isEmpty() ? m_lang : " (" % m_lang % ")";
		return name;
	}
	int id() const {return m_id;}
	bool operator == (const Stream &rhs) const {
		return m_title == rhs.m_title && m_lang == rhs.m_lang && m_codec == rhs.m_codec
				&& m_id == rhs.m_id && m_type == rhs.m_type && m_selected == rhs.m_selected;
	}
	bool isSelected() const { return m_selected; }
	const QString &codec() const { return m_codec; }
	const QString &title() const { return m_title; }
	Type type() const { return m_type; }
	bool isExternal() const { return !m_fileName.isEmpty(); }
	bool isDefault() const { return m_default; }
	bool isAlbumArt() const { return m_albumart; }
private:
	friend class MpMessage;
	friend class PlayEngine;
	Type m_type = Unknown;
	int m_id = -1;
	QString m_title, m_lang, m_fileName, m_codec;
	bool m_selected = false, m_default = false, m_albumart = false;
};

typedef QMap<int, Stream> StreamList;

struct Chapter {
	int time() const { return m_time; }
	QString name() const {return m_name;}
	int id() const {return m_id;}
	bool operator == (const Chapter &rhs) const {
		return m_id == rhs.m_id && m_name == rhs.m_name;
	}
private:
	friend class PlayEngine;
	QString m_name;
	int m_id = -2, m_time = 0;
};

typedef QVector<Chapter> ChapterList;

struct Edition {
	QString name() const { return m_name; }
	int id() const { return m_id; }
	bool isSelected() const { return m_selected; }
	bool operator == (const Edition &rhs) const { return m_id == rhs.m_id && m_selected == rhs.m_selected; }
private:
	friend class PlayEngine;
	int m_id = 0;
	QString m_name;
	bool m_selected = false;
};

typedef QVector<Edition> EditionList;

class TrackInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(int current READ current NOTIFY currentChanged)
	Q_PROPERTY(int count READ count NOTIFY countChanged)
	Q_PROPERTY(int length READ count NOTIFY countChanged)
	Q_PROPERTY(QString currentText READ currentText NOTIFY currentChanged)
	Q_PROPERTY(QString countText READ countText NOTIFY countChanged)
public:
	TrackInfoObject(QObject *parent = nullptr): QObject(parent) {}
	int current() const { return m_current; }
	int count() const { return m_count; }
	void setCount(int count) { if (_Change(m_count, count)) emit countChanged(); }
	void setCurrent(int current) { if (_Change(m_current, current)) emit currentChanged(); }
	QString currentText() const { return toString(m_current); }
	QString countText() const { return toString(m_count); }
signals:
	void currentChanged();
	void countChanged();
private:
	static QString toString(int i) { return i < 1 ? _L("-") : QString::number(i); }
	int m_current = -2;
	int m_count = 0;
};

class ChapterInfoObject : public TrackInfoObject {
	Q_OBJECT
public:
	ChapterInfoObject(const PlayEngine *engine, QObject *parent = nullptr);
	Q_INVOKABLE int time(int i) const;
	Q_INVOKABLE QString name(int i) const;
private:
	const PlayEngine *m_engine = nullptr;
};

class AudioTrackInfoObject : public TrackInfoObject {
	Q_OBJECT
public:
	AudioTrackInfoObject(const PlayEngine *engine, QObject *parent = nullptr);
};

class SubtitleTrackInfoObject : public TrackInfoObject {
	Q_OBJECT
public:
	SubtitleTrackInfoObject(QObject *parent = nullptr): TrackInfoObject(parent) {}
	void set(const QStringList &tracks) { m_tracks = tracks; setCount(m_tracks.size()); }
	void setCurrentIndex(int idx) { setCurrent(idx+1); }
	Q_INVOKABLE QString name(int i) const { return m_tracks.value(i); }
private:
	QStringList m_tracks;
};

#endif // MEDIAMISC_HPP

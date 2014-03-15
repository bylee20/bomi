#ifndef MRL_HPP
#define MRL_HPP

#include "stdafx.hpp"

class Record;

class Mrl {
public:
	Mrl() {}
	Mrl(const QUrl &url);
	Mrl(const QString &location, const QString &name = QString());
	bool operator == (const Mrl &rhs) const {return m_loc == rhs.m_loc;}
	bool operator != (const Mrl &rhs) const {return !(*this == rhs);}
	bool operator < (const Mrl &rhs) const {return m_loc < rhs.m_loc;}
	QString location() const { auto loc = toLocalFile(); return loc.isEmpty() ? m_loc : loc; }
	QString toString() const { return m_loc; }
	bool isLocalFile() const {return m_loc.startsWith(_L("file://"), Qt::CaseInsensitive);}
	bool isDvd() const {return m_loc.startsWith(_L("dvdnav://"), Qt::CaseInsensitive);}
	bool isBluray() const { return m_loc.startsWith(_L("bd://"), Qt::CaseInsensitive); }
	bool isDisc() const;
	QString scheme() const {return m_loc.left(m_loc.indexOf(_L("://")));}
	QString toLocalFile() const {return isLocalFile() ? m_loc.right(m_loc.size() - 7) : QString();}
	QString fileName() const;
	bool isPlaylist() const;
	QString displayName() const;
	bool isEmpty() const;
	QString suffix() const;
	QString name() const { return m_name; }
	bool isImage() const;
	void setName(const QString &name) { m_name = name; }
	static Mrl fromString(const QString str) { Mrl mrl; mrl.m_loc = str; return mrl; }
	static Mrl fromDisc(const QString &scheme, const QString &device, int title = -1);
	QString device() const;
	QByteArray toLocal8Bit() const { return m_loc.toLocal8Bit(); }
private:
	QString m_loc = {};
	QString m_name;
};

Q_DECLARE_METATYPE(Mrl)

#endif // MRL_HPP

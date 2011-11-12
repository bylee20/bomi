#ifndef MRL_HPP
#define MRL_HPP

#include <QtCore/QUrl>

class Record;

class Mrl {
public:
	Mrl() {}
	Mrl(const QString &location);
	Mrl(const Mrl &other): m_loc(other.m_loc) {}
	bool operator == (const Mrl &rhs) const {return m_loc == rhs.m_loc;}
	bool operator != (const Mrl &rhs) const {return !(*this == rhs);}
	Mrl &operator=(const Mrl &rhs) { if (this != &rhs) m_loc = rhs.m_loc; return *this;}
	bool operator < (const Mrl &rhs) const {return m_loc < rhs.m_loc;}
	QString toString() const {return m_loc;}
	bool isLocalFile() const {return m_loc.startsWith("file://", Qt::CaseInsensitive);}
	bool isDVD() const {return m_loc.startsWith("dvd://", Qt::CaseInsensitive);}
	QString scheme() const {return m_loc.left(m_loc.indexOf("://"));}
	QString toLocalFile() const {return isLocalFile() ? m_loc.right(m_loc.size() - 7) : QString();}
	QString fileName() const;
	bool isPlaylist() const;
	QString displayName() const;
	bool isEmpty() const {return m_loc.isEmpty();}
private:
	QString m_loc;
};

#endif // MRL_HPP

#ifndef MPMESSAGE_HPP
#define MPMESSAGE_HPP

#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QStringBuilder>

void mp_msg_va2(int mod, int lev, const char *format, va_list va);

struct Stream {
	QString name() const {
		QString name = m_title;
		if (!m_lang.isEmpty())
			name += name.isEmpty() ? m_lang : " (" % m_lang % ")";
		return name;
	}
	inline int id() const {return m_id;}
private:
	friend class MpMessage;
	int m_id = -1;
	QString m_title;
	QString m_lang;
};

typedef QMap<int, Stream> StreamList;

class MpMessage {
public:
	MpMessage();
	virtual ~MpMessage();
	static void add(MpMessage *parser);
	static void clear();
protected:
	struct Id {
		Id() {}
		Id(const QString &name, const QString &value)
			: name(name), value(value) {}
		QString name = {}, value = {};
	};
	static auto same(const QString &s1, const char *l1) -> bool {
		return s1.compare(QLatin1String(l1), Qt::CaseSensitive) == 0;
	}
	static Id id(const QString &line);
	virtual bool parse(const QString &line);
	virtual bool parse(const Id &id);
	static bool getStream(const Id &id, const char *category, const char *idtext, StreamList &streams);
private:
	static void _parse(const QString &line);
	friend void mp_msg_va2(int mod, int lev, const char *format, va_list va);
	static QList<MpMessage*> parsers;
};

#endif // MPMESSAGE_HPP

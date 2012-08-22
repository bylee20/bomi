#ifndef RECORD_HPP
#define RECORD_HPP

#include <QtCore/QSettings>
#include <QtGui/QKeySequence>
#include <QtCore/QStringBuilder>

static inline QLatin1String _L(const char *s) {return QLatin1String(s);}
static inline QLatin1Char _L(char c) {return QLatin1Char(c);}

class Record : public QSettings {
//	template<typename T>
//	QStringList toStringList(const QList<T> &list) {
//		QStringList ret;
//		ret.reserve(list.size());
//		for (int i=0; i<list.size(); ++i)
//			ret.push_back(list[i].toString());
//		return ret;
//	}

//	template<typename T>
//	QList<T> fromStringList(const QStringList &list) {
//		QList<T> ret;
//		ret.reserve(list.size());
//		for (int i=0; i<list.size(); ++i)
//			ret.push_back(T::fromString(list[i]));
//		return ret;
//	}
public:
	Record() {}
	Record(const QString &root): m_root(root) {
		if (!m_root.isEmpty())
			beginGroup(root);
	}
	~Record() {
		if (!m_root.isEmpty())
			endGroup();
	}
	template <typename T>
	void write(const QString &key, const T &value) {setValue(key, toVariant<T>(value));}
	template <typename T>
	void writeEnum(const QString &key, const T &value) {setValue(key, value.name());}
//	template <typename T>
//	void writeList(const QString &key, const QList<T> &list) {
//		QList<QVariant> values;
//		values.reserve(list);
//		for (int i=0; i<values.size(); ++i)
//		beginWriteArray(key, list.size());
//		for (int i=0; i<list.size(); ++i) {
//			setArrayIndex(i);
//			write<T>(key, list[i]);
//		}
//		endArray();
//	}
	template <typename T> void write(const char *key, const T &value) {write<T>(_L(key), value);}
	template <typename T> void write(const char *key, const QList<T> &value) {
		setValue(_L(key) % _L("_exists"), true);
		beginWriteArray(key, value.size());
		for (int i=0; i<value.size(); ++i) {setArrayIndex(i); write<T>("data", value[i]);}
		endArray();
	}
	template <typename T> void writeEnum(const char *key, const T &value) {write(_L(key), value.name());}
//	template <typename T> void writeList(const char *key, const QList<T> &list) {write<T>(_LS(key), list);}
	template <typename T> QList<T> read(QList<T> &v, const char *key) {
		if (value(_L(key) % _L("_exists"), false).toBool()) {
			v.clear();
			const int size = beginReadArray(key);
			v.reserve(size);
			T t;
			for (int i=0; i<size; ++i) {
				setArrayIndex(i);
				read<T>(t, "data");
				v.append(t);
			}
			endArray();
		}
		return v;
	}

	template <typename T> T read(const char *key) {return value(_L(key)).template value<T>();}
	template <typename T>
	T read(const char *key, const T &def) {return value(_L(key), toVariant<T>(def)).template value<T>();}
	template <typename T>
	T &read(T &value, const char *key) {return value = this->value(_L(key), toVariant<T>(value)).template value<T>();}
	template <typename T>
	T readEnum(const char *key, const T &def = T()) {return T::from(read(key, def.name()));}
	template <typename T>
	T &readEnum(T &value, const char *key) {value.set(read(key, value.name())); return value;}
//	template <typename T>
//	T readList(const QString &key, const QList<T> &def = QList<T>()) {
//		const int size = beginReadArray(key);
//		if (size <= 0)
//			return def;
//		const QStringList keys = set.value(_LS("shortcut"), QStringList(_LS("none"))).toStringList();
//		if (keys.size() != 1 || keys[0] != _LS("none"))
//			(*it)->setShortcuts(fromStringList<QKeySequence>(keys));
//		set.endGroup();
//	}

private:
//	template <typename T>
//	QList<T> fromVariant(const QVariant &data) {
//		if (data.type() != QVariant::List)
//			return QList<T>();
//		const QList<QVariant> list = data.toList();
//		QList<T> ret;
//		ret.reserve(list.size());
//		for (int i=0; i<ret.size(); ++i)

//	}

	template <typename T> T fromVariant(const QVariant &data) {return data.value<T>();}
	template <typename T> QVariant toVariant(const T &t) {return QVariant::fromValue(t);}
	const QString m_root = {};
};

template<> inline QKeySequence Record::fromVariant(const QVariant &data) {return QKeySequence::fromString(data.toString());}
template<> inline QVariant Record::toVariant(const QKeySequence &seq) {return seq.toString();}


#define RECORD_READ(rec, val) {val = rec.read(#val, val);}
#define RECORD_READ_ENUM(rec, val) {val = rec.readEnum(#val, val);}
#define RECORD_WRITE(rec, val) {rec.write(#val, val);}
#define RECORD_WRITE_ENUM(rec, val) {rec.writeEnum(#val, val);}

#endif // RECORD_HPP

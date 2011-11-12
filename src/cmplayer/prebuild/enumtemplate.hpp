#ifndef ENUMTEMPLATE1_HPP
#define ENUMTEMPLATE1_HPP

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>

#define __ENUM_CLASS ListEnumTemplate
#define __ENUM_COUNT 1
#define __DEC_ENUM_VALUES static const __ENUM_CLASS Item0;
#define __DEF_DESCRIPTION Q_UNUSED(id); return QString();
#define __DEC_NAME_ARRAY QString name[count];
#define __DEF_ID_COMPATIBLE return 0 <= id && id < count;
#define __DEFAULT_ID 0
namespace Enum {

class __ENUM_CLASS {
	Q_DECLARE_TR_FUNCTIONS(__ENUM_CLASS)
public:
	typedef QList<__ENUM_CLASS> List;
	static const int count = __ENUM_COUNT;
__DEC_ENUM_VALUES
	__ENUM_CLASS(): m_id(__DEFAULT_ID) {}
	__ENUM_CLASS(const __ENUM_CLASS &rhs): m_id(rhs.m_id) {}
	__ENUM_CLASS &operator = (const __ENUM_CLASS &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const __ENUM_CLASS &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const __ENUM_CLASS &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const __ENUM_CLASS &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {__DEF_ID_COMPATIBLE}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static __ENUM_CLASS from(const QString &name, const __ENUM_CLASS &def = __ENUM_CLASS()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? __ENUM_CLASS(*it) : def;
	}
	static __ENUM_CLASS from(int id, const __ENUM_CLASS &def = __ENUM_CLASS()) {
		return isCompatible(id) ? __ENUM_CLASS(id) : def;
	}
	static QString description(int id) {
__DEF_DESCRIPTION
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		__DEC_NAME_ARRAY
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	__ENUM_CLASS(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	__ENUM_CLASS(int id): m_id(id) {}
	int m_id;
};

}

inline bool operator == (int lhs, const Enum::__ENUM_CLASS &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::__ENUM_CLASS &rhs) {return lhs != rhs.id();}

#endif // ENUMTEMPLATE_HPP

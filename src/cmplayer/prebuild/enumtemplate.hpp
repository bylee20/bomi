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

class __ENUM_CLASS : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(__ENUM_CLASS)
public:
	typedef QList<__ENUM_CLASS> List;
	static const int count = __ENUM_COUNT;
__DEC_ENUM_VALUES
	__ENUM_CLASS(): EnumClass(__DEFAULT_ID) {}
	__ENUM_CLASS(const __ENUM_CLASS &rhs): EnumClass(rhs) {}
	__ENUM_CLASS &operator = (const __ENUM_CLASS &rhs) {setId(rhs.id()); return *this;}
	__ENUM_CLASS &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	__ENUM_CLASS(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = QLatin1String(name), id);
		_map.list.append(*this);
	}
	__ENUM_CLASS(int id): EnumClass(id) {}
};

}

#endif // ENUMTEMPLATE_HPP

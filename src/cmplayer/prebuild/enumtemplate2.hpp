//#ifndef ENUMTEMPLATE2_HPP
//#define ENUMTEMPLATE2_HPP

//#include <QtCore/QCoreApplication>
//#include <QtCore/QMap>

//#define __ENUM_CLASS MapEnumTemplate
//#define __ENUM_COUNT 1
//#define __DEC_ENUM_VALUES static const __ENUM_CLASS Item0;
//#define __DEF_DESCRIPTION Q_UNUSED(id); return QString();

//namespace Enum {

//class __ENUM_CLASS {
//	Q_DECLARE_TR_FUNCTIONS(__ENUM_CLASS)
//public:
//	static const int count = __ENUM_COUNT;
//__DEC_ENUM_VALUES
//	__ENUM_CLASS(): m_id(0) {}
//	__ENUM_CLASS(const __ENUM_CLASS &rhs): m_id(rhs.m_id) {}
//	__ENUM_CLASS &operator=(const __ENUM_CLASS &rhs) {m_id = rhs.m_id; return *this;}
//	bool operator==(const __ENUM_CLASS &rhs) {return m_id == rhs.m_id;}
//	bool operator!=(const __ENUM_CLASS &rhs) {return m_id != rhs.m_id;}
//	int id() const {return m_id;}
//	QString name() const {return map().name[m_id];}
//	QString description() const {return description(m_id);}
//	static bool isCompatible(int id) {return map().name.contains(id);}
//	static bool isCompatible(const QString &name) {return map().value.contains(name);}
//	static __ENUM_CLASS fromName(const QString &name, bool *ok = 0) {
//		__ENUM_CLASS ret;
//		QMap<QString, int>::const_iterator it = map().value.find(name);
//		if (ok)
//			*ok = (it != map().value.end());
//		ret.m_id = (it != map().value.end()) ? *it : 0;
//		return ret;
//	}
//	static __ENUM_CLASS fromId(int id, bool *ok) {
//		__ENUM_CLASS ret;
//		const bool comp = isCompatible(id);
//		if (ok)
//			*ok = comp;
//		ret.m_id = comp ? id : 0;
//		return ret;
//	}
//	static QString description(int id) {
//__DEF_DESCRIPTION
//	}
//private:
//	struct Map {
//		QMap<int, QString> name;
//		QMap<QString, int> value;
//	};
//	static Map _map;
//	static const Map &map() {return _map;}
//	__ENUM_CLASS(int id, const char *name): m_id(id) {
//		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
//	}
//	int m_id;
//};

//}

//#endif // ENUMTEMPLATE2_HPP

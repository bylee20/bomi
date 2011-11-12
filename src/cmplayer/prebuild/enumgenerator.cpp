#include "enumgenerator.hpp"
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

struct EnumData {
	EnumData(): continuous(true) {}
	QString name;
	struct Item {
		QString name;
		QString value;
		QString desc;
	};
	bool continuous;
	QList<Item> items;
};

static QString getTemplate(const QString &fileName) {
	QFile file(fileName);
	file.open(QFile::ReadOnly);
	Q_ASSERT(file.isOpen());
	QString ret;
	while (!file.atEnd()) {
		const QByteArray bytes = file.readLine();
		if (bytes.startsWith("#") || bytes.trimmed().isEmpty())
			continue;
		ret += bytes;
	}
	return ret;
}

static QString hpp() {
	static QString tmp;
	if (tmp.isEmpty())
		tmp = getTemplate("enumtemplate.hpp");
	return tmp;
}

static QString cpp() {
	static QString tmp;
	if (tmp.isEmpty())
		tmp = getTemplate("enumtemplate.cpp");
	return tmp;
}

static QString addOne(const QString &value) {
	int idx = value.size()-1;
	for (; idx >= 0; --idx) {
		const ushort ucs = value[idx].unicode();
		if (!('0' <= ucs && ucs <= '9'))
			break;
	}
	if (idx < 0)
		return QString::number(value.toInt() + 1);
	else
		return value.left(idx+1) += QString::number(value.mid(idx+1).toInt() + 1);
}

static QList<EnumData> readEnums() {
	QFile file("enum-list");
	file.open(QFile::ReadOnly);
	Q_ASSERT(file.isOpen());

	QList<EnumData> data;
#define RX_VARIABLE "[a-zA-Z_][a-zA-Z0-9_]*"
	static QRegExp rxName("^Name:("RX_VARIABLE")$");
	static QRegExp rxHasValue("^("RX_VARIABLE")=([^\\-]+)$");
#undef RX_VARIABLE
	while (!file.atEnd()) {
		QByteArray bytes = file.readLine().trimmed();
		if (bytes.startsWith("#") || bytes.isEmpty())
			continue;
		QString line(bytes);
		if (rxName.indexIn(line) != -1) {
			data.append(EnumData());
			data.last().name = rxName.cap(1);
		} else if (line.startsWith(QLatin1Char('-'))) {
			int idx = line.indexOf(QLatin1Char('-'), 1);
			EnumData::Item item;
			if (idx < 0) {
				item.name = line.mid(1);
			} else {
				item.name = line.mid(1, idx-1);
				item.desc = line.mid(idx+1);
			}
			if (rxHasValue.indexIn(item.name) != -1) {
				item.name = rxHasValue.cap(1);
				item.value = rxHasValue.cap(2);
				if (data.last().continuous)
					data.last().continuous = false;
			} else {
				if (data.last().items.isEmpty())
					item.value = "0";
				else
					item.value = addOne(data.last().items.last().value);
			}
			data.last().items.append(item);
		}
	}
	file.close();
	return data;
}

void EnumGenerator::generate2() {
	qDebug() << "Generate enum files";
	const QList<EnumData> data = readEnums();
	const QString hppTmp = ::hpp();
	const QString cppTmp = ::cpp();
	QFile hppFile("../enums.hpp"), cppFile("../enums.cpp");
	hppFile.open(QFile::WriteOnly | QFile::Truncate);
	cppFile.open(QFile::WriteOnly | QFile::Truncate);
	Q_ASSERT(hppFile.isOpen() && cppFile.isOpen());

	QTextStream s_hpp(&hppFile), s_cpp(&cppFile);
	s_hpp << "#ifndef ENUMS_HPP\n#define ENUMS_HPP\n\n"
		"#include <QtCore/QCoreApplication>\n#include <QtCore/QMap>\n\n";
	s_cpp << "#include \"enums.hpp\"\n\n";
	for (int i=0; i<data.size(); ++i) {
		const EnumData &d = data[i];
		QString hpp = hppTmp;
		QString cpp = cppTmp;
		hpp.replace("__ENUM_CLASS", d.name);
		hpp.replace("__DEFAULT_ID", d.items.first().value);
		cpp.replace("__ENUM_CLASS", d.name);
		hpp.replace("__ENUM_COUNT", QString::number(d.items.size()));
		QString dec_values, def_desc, init_values;
//		def_desc = "\t\tswitch (id) {\n";
		for (int i=0; i<d.items.size(); ++i) {
			static const QString tmp1("\tstatic const %1 %2;\n");
			dec_values += tmp1.arg(d.name).arg(d.items[i].name);
			def_desc += "\t\tif (id == " + d.items[i].name + ".m_id)\n\t\t\treturn ";
			if (d.items[i].desc.isEmpty())
				def_desc += "QString(";
			else
				def_desc += "tr(\"" + d.items[i].desc + "\"";
			def_desc += ");\n";
			static const QString tmp2("const %1 %1::%2(%3, \"%2\");\n");
			init_values += tmp2.arg(d.name, d.items[i].name, d.items[i].value);
		}
		init_values.chop(1);
		def_desc += "\t\treturn QString();";
		hpp.replace("__DEC_ENUM_VALUES", dec_values);
		hpp.replace("__DEF_DESCRIPTION", def_desc);
		cpp.replace("__INIT_ENUM_VALUES", init_values);
		if (d.continuous) {
			hpp.replace("__DEC_NAME_ARRAY", "QString name[count];");
			hpp.replace("__DEF_ID_COMPATIBLE", "return 0 <= id && id < count;");
		} else {
			hpp.replace("__DEC_NAME_ARRAY", "QMap<int, QString> name;");
			hpp.replace("__DEF_ID_COMPATIBLE", "return map().name.contains(id);");
		}
		s_hpp << hpp << endl;
		s_cpp << cpp << endl;
	}

	s_hpp << "#endif" << endl;
}

void EnumGenerator::generate() {
	qDebug() << "Generate enum files";
#define SOURCE_FILE "enum-list"
#define DEST_HPP_FILE "../enums.hpp"
#define DEST_CPP_FILE "../enums.cpp"
	QFile file(SOURCE_FILE);
	file.open(QFile::ReadOnly);
#undef SOURCE_FILE
	Q_ASSERT(file.isOpen());

	QList<EnumData> data;
#define RX_VARIABLE "[a-zA-Z_][a-zA-Z0-9_]*"
	static QRegExp rxName("^Name:("RX_VARIABLE")$");
	static QRegExp rxHasValue("^("RX_VARIABLE")=([0-9]+|0[xX][0-9a-fA-F]+)$");
#undef RX_VARIABLE
	while (!file.atEnd()) {
		QByteArray bytes = file.readLine().trimmed();
		if (bytes.startsWith("#") || bytes.isEmpty())
			continue;
		QString line(bytes);
		if (rxName.indexIn(line) != -1) {
			data.append(EnumData());
			data.last().name = rxName.cap(1);
		} else if (line.startsWith(QLatin1Char('-'))) {
			int idx = line.indexOf(QLatin1Char(':'));
			EnumData::Item item;
			if (idx < 0) {
				item.name = line.mid(1);
			} else {
				item.name = line.mid(1, idx-1);
				item.desc = line.mid(idx+1);
			}
			if (rxHasValue.indexIn(item.name) != -1) {
				item.name = rxHasValue.cap(1);
				item.value = rxHasValue.cap(2);
			}
			data.last().items.append(item);
		}
	}

	file.close();

	file.setFileName(DEST_HPP_FILE);
#undef DEST_HPP_FILE
	file.open(QFile::WriteOnly | QFile::Truncate);
	Q_ASSERT(file.isOpen());


	QTextStream out(&file);
	out << "#ifndef ENUMS_HPP\n";
	out << "#define ENUMS_HPP\n\n";
	out << "#include <QtCore/QCoreApplication>\n#include <QtCore/QMap>\n#include <QtCore/QMetaType>\n\n"
		"namespace Enum {\n\n" << endl;
	for (int i=0; i<data.size(); ++i) {
		const EnumData &d = data[i];

		out << "class " << d.name << " {\n"
			<< "\tQ_DECLARE_TR_FUNCTIONS(" << d.name << ")\n"
			<< "public:\n"
			<< "\tstatic const int count = " << d.items.size() << ';' << endl;

		out << "\tenum Value {\n";
		bool hasValue = false;
		for (int i=0; i<d.items.size(); ++i) {
			out << "\t\t" << d.items[i].name;
			if (!d.items[i].value.isEmpty()) {
				hasValue = true;
				out << '=' << d.items[i].value;
			}
			if (i != d.items.size()-1)
				out << ",";
			out << '\n';
		}
		out << "\t};" << endl;

		const QString item0 = d.items.first().name;
		out << '\t' << d.name << "(): m_value(" << item0 << ") {}\n";
		out << '\t' << d.name << "(Value value): m_value(value) {}\n";
		out <<
			"\tValue value() const {return m_value;}\n"
			"\tQString description() const {return description(m_value);}\n"
			"\tint toInt() const {return (int)m_value;}\n"
			"\tQString toString() const {return toString(m_value);}\n"
			"\tstatic " << d.name << " fromString(const QString &str) "
				"{return map.s2v.value(str, " << item0 << ");}\n"
			"\tstatic " << d.name << " fromInt(int v) "
				"{return isCompatible(v) ? (Value)v : " << item0 << ";}\n"
			"\tstatic bool isCompatible(const QString &string) {return map.s2v.contains(string);}\n";
		if (hasValue) {
			out << "\tstatic QString toString(int v) {return map.v2s.value((Value)v, QString());}\n"
				"\tstatic bool isCompatible(int v) {return map.v2s.contains((Value)v);}\n";
		} else {
			out << "\tstatic QString toString(int v) "
					"{return isCompatible(v) ? map.v2s[v] : QString();}\n"
				"\tstatic bool isCompatible(int v) {return (0 <= v && v < count);}\n";
		}
		out << "\tstatic QString description(int value) {\n"
			"\t\tswitch (value) {\n";
		for (int i=0; i<d.items.size(); ++i) {
			out << "\t\tcase " << d.items[i].name << ": return tr(\"" << d.items[i].desc << "\");\n";
		}
		out << "\t\tdefault: return QString();\n\t\t}\n\t}\n";

		out << "private:\n"
			"\tstruct Map {\n"
			"\t\tMap() {\n";
		for (int i=0; i<d.items.size(); ++i) {
			const QString &item = d.items[i].name;
			out << "\t\t\ts2v.insert(v2s[" << item
				<< "] = QLatin1String(\"" << item << "\"), " << item << ");\n";
		}
		out << "\t\t}\n";
		out << "\t\t" << (hasValue ? "QMap<Value, QString> v2s"
				: "QString v2s[count]") << ";\n"
			"\t\tQMap<QString, Value> s2v;\n\t};\n";
		out << "\tstatic const Map map;\n\tValue m_value;\n};\n" << endl;
	}
	out << "}\n\n";
	for (int i=0; i<data.size(); ++i) {
		out << "Q_DECLARE_METATYPE(Enum::" << data[i].name << ")\n";
	}
	out << "\n#endif" << endl;
	out.setDevice(0);
	file.close();
	file.setFileName(DEST_CPP_FILE);
#undef DEST_CPP_FILE
	file.open(QFile::WriteOnly | QFile::Truncate);
	Q_ASSERT(file.isOpen());
	out.setDevice(&file);
	out << "#include \"enums.hpp\"\n\nnamespace Enum {\n\n";
	for (int i=0; i<data.size(); ++i) {
		const EnumData &d = data[i];
		out << "const " << d.name << "::Map " << d.name << "::map;\n\n";
	}
	out << "}\n";
}

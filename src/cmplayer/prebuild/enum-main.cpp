#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

struct EnumData {
	struct Item {string name, value, desc;};
	EnumData() {continuous = true;}
	string name;
	bool continuous;
	vector<Item> items;
};

static inline std::string trim(const std::string &str) {
	const string spaces(" \t\f\v\n\r");
	const int from = str.find_first_not_of(spaces);
	const int to = str.find_last_not_of(spaces);
	return (from <= to && from != string::npos) ? str.substr(from, to+1-from) : string();
}

static string getTemplate(const string &fileName) {
	fstream in;
	in.open(fileName.c_str(), ios::in);
	assert(in.is_open());
	string line;
	stringstream out;
	while (std::getline(in, line)) {
		cout << trim(line) << endl;
		if (!trim(line).empty() && line[0] != '#') {
			out << line << endl;
		}
	}
	return out.str();
}

static string hpp() {
	static string tmp;
	return tmp.empty() ? (tmp = getTemplate("enumtemplate.hpp")) : tmp;
}

static string cpp() {
	static string tmp;
	return tmp.empty() ? (tmp = getTemplate("enumtemplate.cpp")) : tmp;
}

static inline int toInt(const string &s) {
	stringstream str;
	str.str(s);
	int number;
	str >> number;
	return number;
}

static inline string toString(int n) {
	stringstream str;
	str << n;
	return str.str();
}

static string addOne(const string &value) {
	int idx = value.size()-1;
	for (; idx >= 0; --idx) {
		const char c = value[idx];
		if (!('0' <= c && c <= '9'))
			break;
	}
	stringstream str;
	str << (toInt(idx < 0 ? value : value.substr(idx+1))+1);
	return value.substr(0, idx+1) + str.str();
}

static vector<EnumData> readEnums() {
	fstream in;
	in.open("enum-list", ios::in);
	assert(in.is_open());

	vector<EnumData> data;
	string line;
	while (getline(in, line)) {
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		if (line[0] == '+') {
			data.push_back(EnumData());
			data.back().name = line.substr(1);
		} else if (line[0] == '-') {
			const string::size_type desc_idx = line.find('-', 1);
			const string::size_type value_idx = line.find('=', 1);

			data.back().items.push_back(EnumData::Item());
			EnumData::Item &item = data.back().items.back();
			if (value_idx != string::npos)
				item.name = line.substr(1, value_idx - 1);
			else if (desc_idx != string::npos)
				item.name = line.substr(1, desc_idx - 1);
			else
				item.name = line.substr(1);
			if (value_idx != string::npos) {
				item.value = line.substr(value_idx + 1, desc_idx - (value_idx + 1));
				data.back().continuous = false;
			} else {
				const size_t size = data.back().items.size();
				item.value = size == 1u ? string("0") : addOne(data.back().items[size-2].value);
			}
			if (desc_idx != string::npos)
				item.desc = line.substr(desc_idx + 1);
		}
	}
	return data;
}

static string &replace(string &str, const string &substr, const string &overwrite) {
	string::size_type from = 0;
	for (;;) {
		from = str.find(substr, from);
		if (from == str.npos)
			break;
		str.replace(from, substr.size(), overwrite);
		from += overwrite.size();
	}
	return str;
}

static void generate() {
	cout << "Generate enum files" << endl;
	const vector<EnumData> data = readEnums();
	const string hppTmp = ::hpp();
	const string cppTmp = ::cpp();

	fstream s_hpp, s_cpp;
	s_hpp.open("../enums.hpp", ios::out);
	s_cpp.open("../enums.cpp", ios::out);
	assert(s_hpp.is_open() && s_cpp.is_open());

	s_hpp << "#ifndef ENUMS_HPP\n#define ENUMS_HPP\n\n"
		"#include <QtCore/QCoreApplication>\n#include <QtCore/QMap>\n\n";
	s_cpp << "#include \"enums.hpp\"\n\n";

	for (vector<EnumData>::const_iterator it = data.begin(); it != data.end(); ++it) {
		string hpp = hppTmp;
		string cpp = cppTmp;
		replace(hpp, "__ENUM_CLASS", it->name);
		replace(hpp, "__DEFAULT_ID", it->items.front().value);
		replace(cpp, "__ENUM_CLASS", it->name);
		replace(hpp, "__ENUM_COUNT", toString(it->items.size()));
		string decl_values, defa_desc, init_values;
		for (vector<EnumData::Item>::const_iterator iit = it->items.begin(); iit != it->items.end(); ++iit) {
			string tmp1("\tstatic const %1 %2;\n");
			decl_values += replace(replace(tmp1, "%1", it->name), "%2", iit->name);

			defa_desc += "\t\tif (id == " + iit->name + ".m_id)\n\t\t\treturn ";
			defa_desc += iit->desc.empty() ? "QString();\n" : "tr(\"" + iit->desc + "\");\n";

			string tmp2("const %1 %1::%2(%3, \"%2\");\n");
			init_values += replace(replace(replace(tmp2, "%1", it->name), "%2", iit->name), "%3", iit->value);
		}
		init_values.erase(init_values.size()-1);
		defa_desc += "\t\treturn QString();";
		replace(hpp, "__DEC_ENUM_VALUES", decl_values);
		replace(hpp, "__DEF_DESCRIPTION", defa_desc);
		replace(cpp, "__INIT_ENUM_VALUES", init_values);
		if (it->continuous) {
			replace(hpp, "__DEC_NAME_ARRAY", "QString name[count] = {};");
			replace(hpp, "__DEF_ID_COMPATIBLE", "return 0 <= id && id < count;");
		} else {
			replace(hpp, "__DEC_NAME_ARRAY", "QMap<int, QString> name = {};");
			replace(hpp, "__DEF_ID_COMPATIBLE", "return map().name.contains(id);");
		}
		s_hpp << hpp << endl;
		s_cpp << cpp << endl;
	}

	s_hpp << "#endif" << endl;
}

int main() {
	generate();
	return 0;
}

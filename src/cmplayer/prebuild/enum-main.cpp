#include <iostream>
#include <algorithm>
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

static const string hppTmp = R"(
enum class __ENUM_NAME : int {
__ENUM_VALUES
};

inline bool operator == (__ENUM_NAME e, int i) { return (int)e == i; }
inline bool operator != (__ENUM_NAME e, int i) { return (int)e != i; }
inline bool operator == (int i, __ENUM_NAME e) { return (int)e == i; }
inline bool operator != (int i, __ENUM_NAME e) { return (int)e != i; }
inline int operator & (__ENUM_NAME e, int i) { return (int)e & i; }
inline int operator & (int i, __ENUM_NAME e) { return (int)e & i; }
inline int &operator &= (int &i, __ENUM_NAME e) { return i &= (int)e; }
inline int operator ~ (__ENUM_NAME e) { return ~(int)e; }
inline int operator | (__ENUM_NAME e, int i) { return (int)e | i; }
inline int operator | (int i, __ENUM_NAME e) { return (int)e | i; }
inline int operator | (__ENUM_NAME e1, __ENUM_NAME e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, __ENUM_NAME e) { return i |= (int)e; }
inline bool operator > (__ENUM_NAME e, int i) { return (int)e > i; }
inline bool operator < (__ENUM_NAME e, int i) { return (int)e < i; }
inline bool operator >= (__ENUM_NAME e, int i) { return (int)e >= i; }
inline bool operator <= (__ENUM_NAME e, int i) { return (int)e <= i; }
inline bool operator > (int i, __ENUM_NAME e) { return i > (int)e; }
inline bool operator < (int i, __ENUM_NAME e) { return i < (int)e; }
inline bool operator >= (int i, __ENUM_NAME e) { return i >= (int)e; }
inline bool operator <= (int i, __ENUM_NAME e) { return i <= (int)e; }

template<>
class EnumInfo<__ENUM_NAME> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef __ENUM_NAME Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return __ENUM_COUNT; }
	static const char *name(Enum e) {
		__ENUM_FUNC_NAME
	}
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
__ENUM_FUNC_DESC_CASES
		};
	}
	static constexpr const std::array<Item, __ENUM_COUNT> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, __ENUM_COUNT> info;
};

using __ENUM_NAMEInfo = EnumInfo<__ENUM_NAME>;
)";

static const string cppTmp = R"(
const std::array<__ENUM_NAMEInfo::Item, __ENUM_COUNT> __ENUM_NAMEInfo::info{{
__ENUM_INFOS
}};
)";

static void generate() {
	cout << "Generate enum files" << endl;
	const auto enums = readEnums();
	string hpp = R"(
#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QCoreApplication>
#include <array>

template<typename T> class EnumInfo { static constexpr int size() { return 0; } double dummy; };
)";
	string cpp = "#include \"enums.hpp\"";
	for (const EnumData &data : enums) {
		string htmpl = hppTmp;
		string ctmpl = cppTmp;
		replace(htmpl, "__ENUM_NAME", data.name);
		string value, infos, cases;
		for (const EnumData::Item &item : data.items) {
			value += "\t" + item.name + " = (int)" + item.value;
			infos += "\t{" + data.name + "::" + item.name + ", " + '"' + item.name + "\"}";
			cases += "\t\tcase Enum::" + item.name + ": return tr(QT_TRANSLATE_NOOP(\"EnumInfo\", \"" + item.desc + "\"));\n";
			if (&item != &data.items.back()) {
				value += ",\n";
				infos += ",\n";
			}
		}
		cases += "\t\tdefault: return tr(\"\");";
		replace(htmpl, "__ENUM_VALUES", value);
		replace(htmpl, "__ENUM_COUNT", toString(data.items.size()));
		replace(htmpl, "__ENUM_FUNC_DESC_CASES", cases);
		if (data.continuous)
			replace(htmpl, "__ENUM_FUNC_NAME", R"(return 0 <= e && e < size() ? info[(int)e].name : "";)");
		else
			replace(htmpl, "__ENUM_FUNC_NAME", R"(auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? it->name : "";)");

		replace(ctmpl, "__ENUM_NAME", data.name);
		replace(ctmpl, "__ENUM_COUNT", toString(data.items.size()));
		replace(ctmpl, "__ENUM_INFOS", infos);

		cout << htmpl;
		hpp += htmpl;
		cpp += ctmpl;
	}
	hpp += "\n#endif\n";
	fstream s_hpp, s_cpp;
	s_hpp.open("../enums.hpp", ios::out);
	s_cpp.open("../enums.cpp", ios::out);
	s_hpp << hpp;
	s_cpp << cpp;
}

int main() {
	generate();
	return 0;
}

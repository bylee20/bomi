#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

struct EnumType {
    struct Item {string key, name, value, desc, data;};
    EnumType() {continuous = true;}
    string key, name, desc, data, head, foot, flags;
    bool continuous;
    vector<Item> items;
    int default_ = 0;
};

static inline std::string trim(const std::string &str) {
    const string spaces("     \f\v\n\r");
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

static vector<EnumType> readEnums() {
    fstream in;
    in.open("enum-list", ios::in);
    assert(in.is_open());

    auto substr = [] (std::string &str, const std::string &open, const string &close) {
        auto from = str.find(open);
    if (from == string::npos)
        return string();
    from += open.size();
        auto to   = str.find(close, from);
    if (to == string::npos)
        return string();
        return str.substr(from, to - from);
    };

    vector<EnumType> type;
    string read;
    while (getline(in, read)) {
        string line = trim(read);
        if (line.empty())
            continue;
        bool isType = false;
        if (line[0] == '+')
            isType = true;
        else if (line[0] == '-')
            isType = false;
        else if (line[0] == '!') {
            type.back().head += read.substr(1) + '\n';
            continue;
        } else if (line[0] == '$') {
            if (type.back().foot.empty())
                type.back().foot += '\n';
            type.back().foot += read.substr(1) + '\n';
            continue;
        } else
            continue;
        line = line.substr(1);
        const auto def = line.front() == '*';
        if (def)
            line = line.substr(1);
        const auto key = substr(line, "[[", "]]");
        const auto value = substr(line, "[=", "=]");
        const auto desc = substr(line, "[-", "-]");
        const auto data = substr(line, "[:", ":]");
        const auto flags = substr(line, "[~", "~]");
        const auto idx = {line.find("[["), line.find("[="), line.find("[-"), line.find("[:"), line.find("[~")};
        const auto name = line.substr(0, *std::min_element(std::begin(idx), std::end(idx)));
        if (isType) {
            type.push_back(EnumType());
            auto &one = type.back();
            one.name = name;
            one.key = key;
            one.desc = desc;
            one.data = data.empty() ? "QVariant" : data;
            one.flags = flags;
        } else {
            auto &items = type.back().items;
            items.push_back(EnumType::Item());
            auto &item = items.back();
            item.name = name;
            item.key = key;
            item.desc = desc;
            if (!value.empty()) {
                item.value = value;
                type.back().continuous = false;
            } else {
                const size_t size = items.size();
                item.value = size == 1u ? string("0") : addOne(items[size-2].value);
            }
            if (def)
                type.back().default_ = items.size()-1;
            item.data = data.empty() ? ("(int)" + item.value) : data;
        }
    }
    return type;
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

static string readAll(const string &fileName) {
    std::stringstream buffer;
    fstream file;
    file.open(fileName, ios::in);
    if (!file.is_open())
        return string();
    buffer << file.rdbuf();
    return buffer.str();
}

bool overwrite(const string &fileName, const string &contents) {
    if (readAll(fileName) == contents)
        return false;
    fstream file;
    file.open(fileName, ios::out);
    file << contents;
    cout << "Write to " << fileName << endl;
    cout << contents;
    return true;
}

static string toLower(const string &str) {
    string ret; ret.reserve(str.size());
    for (unsigned int i=0; i<str.size(); ++i)
        ret.push_back(tolower(str[i]));
    return ret;
}

static string toUpper(const string &str) {
    string ret; ret.reserve(str.size());
    for (unsigned int i=0; i<str.size(); ++i)
        ret.push_back(toupper(str[i]));
    return ret;
}

static void write(const EnumType &type) {
    string hpp = readAll("enum_type.hpp");
    string cpp = readAll("enum_type.cpp");;
    string uppers = toUpper(type.name), lowers = toLower(type.name);
    replace(hpp, "__ENUM_NAME", type.name);
    replace(hpp, "__ENUM_KEY", type.key);
    replace(hpp, "__ENUM_DESC", type.desc);
    replace(hpp, "__ENUM_DEFAULT", type.items[type.default_].name);
    replace(hpp, "__ENUM_DATA_TYPE", type.data);
    replace(hpp, "__ENUM_HEADER_CONTENTS", type.head);
    replace(hpp, "__ENUM_FOOTER_CONTENTS", type.foot);
    replace(hpp, "__ENUM_UPPERS", uppers);
    replace(hpp, "__ENUM_LOWERS", lowers);
    replace(hpp, "__ENUM_IS_FLAG", type.flags.empty() ? "0" : "1");
    replace(hpp, "__ENUM_FLAGS_NAME", type.flags);
    
    string value, infos, cases;
    for (const EnumType::Item &item : type.items) {
        value += "    " + item.name + " = (int)" + item.value;
        infos += "    {" + type.name + "::" + item.name + ", " + "u\"" + item.name + "\"_q, u\"" + item.key + "\"_q, " + item.data + "}";
        cases += "        case Enum::" + item.name + ": return qApp->translate(\"EnumInfo\", \"" + item.desc + "\");\n";
        if (&item != &type.items.back()) {
            value += ",\n";
            infos += ",\n";
        }
    }
    cases += "        default: return QString();";
    replace(hpp, "__ENUM_VALUES", value);
    replace(hpp, "__ENUM_COUNT", toString(type.items.size()));
    replace(hpp, "__ENUM_FUNC_DESC_CASES", cases);
    if (type.continuous)
        replace(hpp, "__ENUM_FUNC_ITEM", R"(return 0 <= e && e < size() ? &info[(int)e] : nullptr;)");
    else
        replace(hpp, "__ENUM_FUNC_ITEM", R"(
    auto it = std::find_if(info.cbegin(), info.cend(),
                            [e] (const Item &info)
                            { return info.value == e; });
    return it != info.cend() ? &(*it) : nullptr;
)");

    replace(cpp, "__ENUM_NAME", type.name);
    replace(cpp, "__ENUM_COUNT", toString(type.items.size()));
    replace(cpp, "__ENUM_INFOS", infos);
    replace(cpp, "__ENUM_LOWERS", lowers);
    overwrite("../enum/" + lowers + ".hpp", hpp);
    overwrite("../enum/" + lowers + ".cpp", cpp);
}

static void generate() {
    cout << "Generate enum files" << endl;
    const auto enums = readEnums();
    string hpp = readAll("enums.hpp");
    replace(hpp, "__ENUM_TOTAL_COUNT", toString(enums.size()));
    overwrite("../enum/enums.hpp", hpp);

    string enumIds = 
	"auto _EnumMetaTypeIds() -> const std::array<int, __ENUM_TOTAL_COUNT>&\n"
	"{\n"
    "    static const std::array<int, __ENUM_TOTAL_COUNT> ids = {\n";
    replace(enumIds, "__ENUM_TOTAL_COUNT", toString(enums.size()));
    
    string enumConverter = "auto _EnumNameVariantConverter(int metaType) -> EnumNameVariantConverter\n{\n    EnumNameVariantConverter conv;\n";
    
    string cpp = "#include \"enums.hpp\"\n" ;
    string headers;
    for (const EnumType &type : enums) {
        write(type);
        cpp += "#include \"" + toLower(type.name) + ".hpp\"\n";
        
        string conv = "    if (metaType == qMetaTypeId<__ENUM_NAME>()) {\n"
                      "        conv.variantToName = _EnumVariantToEnumName<__ENUM_NAME>;\n"
                      "        conv.nameToVariant = _EnumNameToEnumVariant<__ENUM_NAME>;\n"
                      "    } else";
        enumConverter += replace(conv, "__ENUM_NAME", type.name);
        enumIds += "        qMetaTypeId<" + type.name  + ">(),\n";
    }
    enumConverter += "\n        return EnumNameVariantConverter();\n    return conv;\n}\n";
    enumIds.erase(enumIds.size() - 2);
    enumIds += "\n    };\n    return ids;\n}";
    cpp += enumConverter;
    cpp += enumIds;
    overwrite("../enum/enums.cpp", cpp);
}

int main() {
    generate();
    return 0;
}

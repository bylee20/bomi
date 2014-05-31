#ifndef SUBTITLE_PARSER_P_HPP
#define SUBTITLE_PARSER_P_HPP

#include "subtitle_parser.hpp"

class TagFormat;
struct RichTextBlock;

class SamiParser : public SubtitleParser {
public:
    auto _parse(Subtitle &sub) -> void;
    auto isParsable() const -> bool;
};

class SubRipParser : public SubtitleParser {
public:
    auto _parse(Subtitle &sub) -> void;
    auto isParsable() const -> bool;
};

class LineParser : public SubtitleParser {
public:
    LineParser(const QString expr): rxLine(expr) {}
    auto isParsable() const -> bool {
        if (skipSeparators())
            return false;
        const int pos = this->pos();
        if (rxLine.indexIn(trim(getLine()).toString()) == -1)
            return false;
        seekTo(pos);
        return true;
    }
protected:
    QRegExp rxLine;
};

class TMPlayerParser : public LineParser {
public:
    TMPlayerParser(): LineParser(u"^\\s*(\\d?\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(.*)$"_q) {}
    auto _parse(Subtitle &sub) -> void;
};

class MicroDVDParser : public LineParser {
public:
    MicroDVDParser(): LineParser(u"^\\{(\\d+)\\}\\{(\\d+)\\}(.*)$"_q) {}
    auto _parse(Subtitle &sub) -> void;
};

#endif // SUBTITLE_PARSER_P_HPP

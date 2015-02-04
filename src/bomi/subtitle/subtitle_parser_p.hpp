#ifndef SUBTITLE_PARSER_P_HPP
#define SUBTITLE_PARSER_P_HPP

#include "subtitle_parser.hpp"

class TagFormat;
struct RichTextBlock;

class SamiParser : public SubtitleParser {
public:
    auto _parse(Subtitle &sub) -> void;
    auto isParsable() const -> bool;
    auto type() const -> SubType { return SubType::SAMI; }
};

class SubRipParser : public SubtitleParser {
public:
    SubRipParser()
        : rx(uR"((^|[\n\r]+)\s*(\d+)s*[\n\r]+\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*)"
             uR"(-->\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*[\n\r]+)"_q) { }
    auto _parse(Subtitle &sub) -> void;
    auto isParsable() const -> bool;
    auto type() const -> SubType { return SubType::SubRip; }
private:
    QRegEx rx;
};

class LineParser : public SubtitleParser {
public:
    LineParser(const QString expr): rxLine(expr) { }
    auto isParsable() const -> bool final;
    auto match(const QString &line) const -> QRegExMatch
        { return rxLine.match(line); }
private:
    QRegEx rxLine;
};

class TMPlayerParser : public LineParser {
public:
    TMPlayerParser()
        : LineParser(uR"(^\s*(\d?\d)\s*:\s*(\d\d)\s*:\s*(\d\d)\s*:\s*(.*)$)"_q)
    { }
    auto _parse(Subtitle &sub) -> void;
    auto type() const -> SubType { return SubType::TMPlayer; }
};

class MicroDVDParser : public LineParser {
public:
    MicroDVDParser(): LineParser(uR"(^\{(\d+)\}\{(\d+)\}(.*)$)"_q) { }
    auto _parse(Subtitle &sub) -> void;
    auto type() const -> SubType { return SubType::MicroDVD; }
};

#endif // SUBTITLE_PARSER_P_HPP

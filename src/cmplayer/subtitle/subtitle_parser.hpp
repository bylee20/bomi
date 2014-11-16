#ifndef SUBTITLE_PARSER_HPP
#define SUBTITLE_PARSER_HPP

#include "subtitle.hpp"

class SubtitleParser : public RichTextHelper {
public:
    virtual ~SubtitleParser() {}
    static auto parse(const QString &file, const QString &enc) -> Subtitle;
    static auto setMsPerCharactor(int msPerChar) -> void
        { SubtitleParser::msPerChar = msPerChar; }
protected:
    virtual bool isParsable() const = 0;
    virtual void _parse(Subtitle &sub) = 0;
    virtual auto type() const -> SubType = 0;
    const QString &all() const { return m_all; }
    auto getLine() const -> QStringRef;
    auto pos() const -> int { return m_pos; }
    auto atEnd() const -> bool { return m_pos >= m_all.size(); }
    auto at(int i) const -> ushort { return m_all.at(i).unicode(); }
    auto seekTo(int pos) const -> void { m_pos = pos; }
    auto skipSeparators() const -> bool
        { return RichTextHelper::skipSeparator(m_pos, m_all); }
    auto file() const -> const QFileInfo& { return m_file; }
    auto append(Subtitle &s, SubComp::SyncType b = SubComp::Time) -> SubComp&;
    static auto predictEndTime(const SubComp::const_iterator &it) -> int;
    static auto predictEndTime(int start, const QString &text) -> int;
    static auto predictEndTime(int start, const QStringRef &text) -> int;
    static auto encodeEntity(const QStringRef &str) -> QString;
    static auto processLine(int &idx, const QString &contents) -> QStringRef;
    static auto components(Subtitle &sub) -> QList<SubComp>&
        { return sub.m_comp; }
    static auto components(const Subtitle &sub) -> const QList<SubComp>&
        { return sub.m_comp; }
    static auto append(SubComp &c, const QString &text, int start) -> void
        { c[start] += RichTextDocument(text); }
    static auto append(SubComp &c, const QString &t, int start, int end) -> void
        { append(c, t, start); c[end]; }
private:
    static int msPerChar;
    QString m_all, m_encoding;
    QFileInfo m_file;
    mutable int m_pos = 0;
};

inline auto SubtitleParser::append(Subtitle &s, SubComp::SyncType b) -> SubComp&
{
    s.m_comp.append(SubComp(type(), m_file, m_encoding, s.m_comp.size() + 1, b));
    return s.m_comp.last();
}

#endif // SUBTITLE_PARSER_HPP

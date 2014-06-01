#ifndef SUBTITLE_PARSER_HPP
#define SUBTITLE_PARSER_HPP

#include "subtitle.hpp"

class SubtitleParser : public RichTextHelper {
public:
    virtual ~SubtitleParser() {}
    static auto parse(const QString &file, const QString &enc) -> Subtitle;
//    auto setEncoding(const QString &enc) -> void {m_enc = enc;}
    static auto setMsPerCharactor(int msPerChar) -> void {SubtitleParser::msPerChar = msPerChar;}
    auto save(const Subtitle &sub, const QString &fileName, const QString &enc) -> bool;
protected:
    virtual bool isParsable() const = 0;
    virtual auto _save(QString &/*save*/, const Subtitle &/*sub*/) -> bool {return false;}
    virtual void _parse(Subtitle &sub) = 0;
    const QString &all() const {return m_all;}
    auto getLine() const -> QStringRef;
    auto pos() const -> int {return m_pos;}
    auto atEnd() const -> bool {return m_pos >= m_all.size();}
    auto at(int i) const -> ushort {return m_all.at(i).unicode();}
    auto seekTo(int pos) const -> void {m_pos = pos;}
    static auto predictEndTime(const SubComp::const_iterator &it) -> int;
    static auto predictEndTime(int start, const QString &text) -> int;
    static auto predictEndTime(int start, const QStringRef &text) -> int;
    static auto encodeEntity(const QStringRef &str) -> QString;
    static auto processLine(int &idx, const QString &contents) -> QStringRef;
    static QList<SubComp> &components(Subtitle &sub) {return sub.m_comp;}
    static const QList<SubComp> &components(const Subtitle &sub) {return sub.m_comp;}
    static auto append(SubComp &c, const QString &text, int start) -> void {c[start] += RichTextDocument(text);}
    static auto append(SubComp &c, const QString &text, int start, int end) -> void {append(c, text, start); c[end];}
    const QFileInfo &file() const {return m_file;}
    auto skipSeparators() const -> bool {return RichTextHelper::skipSeparator(m_pos, m_all);}
    SubComp &append(Subtitle &sub, SubComp::SyncType base = SubComp::Time) {
        sub.m_comp.append(SubComp(m_file, m_encoding, sub.m_comp.size(), base));
        return sub.m_comp.last();
    }
private:
    static int msPerChar;
    QString m_all, m_encoding;
    QFileInfo m_file;
    mutable int m_pos = 0;
};


#endif // SUBTITLE_PARSER_HPP

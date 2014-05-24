#ifndef SUBTITLE_HPP
#define SUBTITLE_HPP

#include "richtextdocument.hpp"
#include "submisc.hpp"

struct SubCapt : public RichTextDocument {
    SubCapt() {index = -1;}
    RichTextDocument &doc() {return *this;}
    const RichTextDocument &doc() const {return *this;}
    inline SubCapt &operator += (const SubCapt &rhs) {RichTextDocument::operator += (rhs); return *this;}
    inline SubCapt &operator += (const RichTextDocument &rhs) {RichTextDocument::operator += (rhs); return *this;}
    inline SubCapt &operator += (const QList<RichTextBlock> &rhs) {RichTextDocument::operator += (rhs); return *this;}
    mutable int index;
};

class SubComp {
public:
    using Map = QMap<int, SubCapt>;
    using It = Map::iterator;
    using ConstIt = Map::const_iterator;
    using iterator = Map::iterator;
    using const_iterator = Map::const_iterator;
//    struct Lang {
//        auto id() const -> QString {
//            if (!name.isEmpty())
//                return name;
//            if (!locale.isEmpty())
//                return locale;
//            return klass;
//        }
//        QString name, locale, klass;
//    };
    enum SyncType {Time, Frame};
    SubComp();
    SubComp(const QFileInfo &file, const QString &enc, int id, SyncType base);
    bool operator == (const SubComp &rhs) const {return m_info.path == rhs.m_info.path && m_klass == rhs.m_klass;}
    bool operator != (const SubComp &rhs) const {return !operator==(rhs);}
    SubCapt &operator[] (int key) { return m_capts[key]; }
    SubCapt operator[] (int key) const { return m_capts[key]; }
    SubComp &unite(const SubComp &other, double frameRate);
    auto united(const SubComp &other, double frameRate) const -> SubComp;

    auto hasWords() const -> bool { for (auto &capt : m_capts) if (capt.hasWords()) return true; return false; }
    auto isEmpty() const -> bool { return m_capts.isEmpty(); }
    auto begin() -> It { return m_capts.begin(); }
    auto end() -> It { return m_capts.end(); }
    auto begin() const -> ConstIt { return m_capts.begin(); }
    auto end() const -> ConstIt { return m_capts.end(); }
    auto cbegin() const -> ConstIt { return m_capts.cbegin(); }
    auto cend() const -> ConstIt { return m_capts.cend(); }
    auto upperBound(int key) -> It { return m_capts.upperBound(key); }
    auto lowerBound(int key) -> It { return m_capts.lowerBound(key); }
    auto upperBound(int key) const -> ConstIt { return m_capts.upperBound(key); }
    auto lowerBound(int key) const -> ConstIt { return m_capts.lowerBound(key); }
    auto insert(int key, const SubCapt &capt) -> It { return m_capts.insert(key, capt); }

    auto name() const -> QString;
    const QString &fileName() const {return m_file;}
    const QString &path() const { return m_info.path; }
    const SubtitleFileInfo &fileInfo() const { return m_info; }
    auto base() const -> SyncType {return m_base;}
    auto isBasedOnFrame() const -> bool {return m_base == Frame;}
//    const Language &language() const {return m_lang;}
    auto language() const -> QString {return m_klass;}
    auto start(int time, double frameRate) const -> const_iterator;
    auto finish(int time, double frameRate) const -> const_iterator;
    static auto msec(int frame, double frameRate) -> int {return qRound(frame/frameRate*1000.0);}
    static auto frame(int msec, double frameRate) -> int {return qRound(msec*0.001*frameRate);}
    auto toTime(int key, double fps) const -> int { return m_base == Time ? key : msec(key, fps); }
    const Map &map() const { return m_capts; }
    auto setLanguage(const QString &lang) -> void { m_klass = lang; }
    auto selection() const -> bool { return m_selection; }
    bool &selection() { return m_selection; }
    auto id() const -> int { return m_id; }
private:
    friend class Parser;
    QString m_file, m_klass;
    SubtitleFileInfo m_info;
    SyncType m_base = Time;
//    Language m_lang;
    Map m_capts;
    bool m_selection = false;
    int m_id = -1;
};

using SubtitleComponentIterator = QMapIterator<int, SubCapt>;

class Subtitle {
public:
    const SubComp &operator[] (int rhs) const {return m_comp[rhs];}
    Subtitle &operator += (const Subtitle &rhs) {m_comp += rhs.m_comp; return *this;}
    auto count() const -> int {return m_comp.size();}
    auto size() const -> int {return m_comp.size();}
    auto isEmpty() const -> bool;
    auto component(double frameRate) const -> SubComp;
    const QList<SubComp> &components() const { return m_comp; }
//    auto start(int time, double frameRate) const -> int;
//    auto end(int time, double frameRate) const -> int;
    auto caption(int time, double frameRate) const -> RichTextDocument;
    auto load(const QString &file, const QString &enc, double accuracy) -> bool;
    auto clear() -> void {m_comp.clear();}
    auto append(const SubComp &comp) -> void {m_comp.append(comp);}
    static auto parse(const QString &fileName, const QString &enc) -> Subtitle;
private:
    friend class SubtitleParser;
    QList<SubComp> m_comp;
};

#endif // SUBTITLE_HPP



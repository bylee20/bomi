#ifndef SUBTITLE_HPP
#define SUBTITLE_HPP

#include "richtextdocument.hpp"
#include "misc/encodinginfo.hpp"

class StreamTrack;

enum class SubType {
    Unknown,
    SAMI,
    SubRip,
    TMPlayer,
    MicroDVD
};

struct SubCapt : public RichTextDocument {
    SubCapt() { index = -1; }
    auto doc() -> RichTextDocument& {return *this;}
    auto doc() const -> const RichTextDocument& {return *this;}
    auto operator += (const SubCapt &rhs) -> SubCapt&
        {RichTextDocument::operator += (rhs); return *this;}
    auto operator += (const RichTextDocument &rhs) -> SubCapt&
        {RichTextDocument::operator += (rhs); return *this;}
    auto operator += (const QList<RichTextBlock> &rhs) -> SubCapt&
        {RichTextDocument::operator += (rhs); return *this;}
    mutable int index;
};

class SubComp {
public:
    using Map = QMap<int, SubCapt>;
    using It = Map::iterator;
    using ConstIt = Map::const_iterator;
    using iterator = Map::iterator;
    using const_iterator = Map::const_iterator;
    enum SyncType { Time, Frame };
    SubComp();
    auto operator == (const SubComp &rhs) const -> bool
        {return m_path == rhs.m_path && m_klass == rhs.m_klass;}
    auto operator != (const SubComp &rhs) const -> bool {return !operator==(rhs);}
    auto operator[] (int key) -> SubCapt& { return m_capts[key]; }
    auto operator[] (int key) const -> SubCapt { return m_capts[key]; }
    auto unite(const SubComp &other, double frameRate) -> SubComp&;
    auto united(const SubComp &other, double frameRate) const -> SubComp;

    auto hasWords() const -> bool
        { for (auto &c : m_capts) if (c.hasWords()) return true; return false; }
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
    auto contains(int key) const -> bool { return m_capts.contains(key); }
    auto name() const -> QString;
    auto fileName() const -> const QString& {return m_file;}
    auto path() const -> const QString& { return m_path; }
    auto base() const -> SyncType {return m_base;}
    auto isBasedOnFrame() const -> bool {return m_base == Frame;}
//    const Language &language() const {return m_lang;}
    auto language() const -> QString {return m_klass;}
    auto start(int time, double frameRate) const -> const_iterator;
    auto finish(int time, double frameRate) const -> const_iterator;
    auto toTime(int key, double fps) const -> int { return m_base == Time ? key : msec(key, fps); }
    auto map() const -> const Map& { return m_capts; }
    auto setLanguage(const QString &lang) -> void { m_klass = lang; }
    auto selection() const -> bool { return m_selection; }
    auto selection() -> bool& { return m_selection; }
    auto id() const -> int { return m_id; }
    auto type() const -> SubType { return m_type; }
    auto toTrack() const -> StreamTrack;
    auto encoding() const -> EncodingInfo { return m_enc; }
    static auto msec(int frame, double fps) -> int {return qRound(frame/fps*1e3);}
    static auto frame(int msec, double fps) -> int {return qRound(msec*1e-3*fps);}
private:
    SubComp(SubType type, const QFileInfo &file, const EncodingInfo &enc, int id, SyncType base);
    friend class SubtitleParser;
    QString m_file, m_klass, m_path;
    EncodingInfo m_enc;
    SyncType m_base = Time;
    Map m_capts;
    bool m_selection = false;
    int m_id = -1;
    SubType m_type = SubType::Unknown;
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
    auto load(const QString &file, const EncodingInfo &enc) -> bool;
    auto clear() -> void {m_comp.clear();}
    auto append(const SubComp &comp) -> void {m_comp.append(comp);}
    static auto parse(const QString &fileName, const EncodingInfo &enc) -> Subtitle;
private:
    friend class SubtitleParser;
    QList<SubComp> m_comp;
};

#endif // SUBTITLE_HPP



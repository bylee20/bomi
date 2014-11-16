#include "streamtrack.hpp"
#include "subtitle/subtitle.hpp"

auto translator_display_language(const QString &iso) -> QString;

SIA _IsAlphabet(ushort c) -> bool
{
    return _InRange<ushort>('a', c, 'z') || _InRange<ushort>('A', c, 'Z');
}

SIA _IsAlphabet(const QString &text) -> bool
{
    for (auto &c : text) {
        if (!_IsAlphabet(c.unicode()))
            return false;
    }
    return true;
}

auto StreamTrack::name() const -> QString
{
    QString name = m_title;
    const QString lang = m_displayLang.isEmpty() ? m_lang : m_displayLang;
    if (!lang.isEmpty())
        name += name.isEmpty() ? lang : " ("_a % lang % ")"_a;
    return name;
}

auto StreamTrack::fromMpvData(const QVariant &mpv) -> StreamTrack
{
    const auto map = mpv.toMap();
    auto type = StreamUnknown;
    switch (map[u"type"_q].toString().at(0).unicode()) {
        case 'v': type = StreamVideo; break;
        case 'a': type = StreamAudio; break;
        case 's': type = StreamSubtitle; break;
        default : return StreamTrack();
    }
    StreamTrack track;
    track.m_type = type;
    track.m_albumart = map[u"albumart"_q].toBool();
    track.m_codec = map[u"codec"_q].toString();
    track.m_default = map[u"default"_q].toBool();
    track.m_id = map[u"id"_q].toInt();
    track.m_lang = map[u"lang"_q].toString();
    if (_InRange(2, track.m_lang.size(), 3) && _IsAlphabet(track.m_lang))
        track.m_displayLang = translator_display_language(track.m_lang);
    track.m_title = map[u"title"_q].toString();
    track.m_fileName = map[u"external-filename"_q].toString();
    if (!track.m_fileName.isEmpty())
        track.m_title = QFileInfo(track.m_fileName).fileName();
    track.m_selected = map[u"selected"_q].toBool();
    return track;
}

auto StreamTrack::fromSubComp(const SubComp &comp) -> StreamTrack
{
    StreamTrack track;
    track.m_type = StreamSubtitle;
    track.m_id = comp.id();
    track.m_lang = comp.language();
    track.m_selected = comp.selection();
    track.m_title = track.m_fileName = comp.fileName();
    switch (comp.type()) {
#define TYPE(t) case SubType::t: track.m_codec = u"" #t ""_q; break;
    TYPE(SAMI);
    TYPE(SubRip);
    TYPE(MicroDVD);
    TYPE(TMPlayer);
    default: break;
#undef TYPE
    }
    return track;
}

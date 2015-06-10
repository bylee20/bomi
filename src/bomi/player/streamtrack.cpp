#include "streamtrack.hpp"
#include "subtitle/subtitle.hpp"
#include "misc/locale.hpp"

SIA type2str(StreamType type) -> QString
{
    switch (type) {
    case StreamVideo:             return u"video"_q;
    case StreamAudio:             return u"audio"_q;
    case StreamSubtitle:          return u"subtitle"_q;
    case StreamInclusiveSubtitle: return u"inclusive-subtitle"_q;
    default:                      return QString();
    }
}

SIA str2type(const QString &type) -> StreamType
{
    if (type.isEmpty())
        return StreamUnknown;
    switch (type.at(0).unicode()) {
        case 'v': return StreamVideo;
        case 'a': return StreamAudio;
        case 's': return StreamSubtitle;
        case 'i': return StreamInclusiveSubtitle;
        default : return StreamUnknown;
    }
}

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

auto StreamTrack::compareMetaData(const StreamTrack &rhs) const -> bool
{
    return m_title == rhs.m_title && m_lang == rhs.m_lang
           && m_codec == rhs.m_codec && m_id == rhs.m_id
           && m_type == rhs.m_type;
}

auto StreamTrack::isExclusive() const -> bool {
    if (m_type != StreamSubtitle)
        return true;
    return !isExternal() || m_file.endsWith(u".ass"_q, Qt::CaseInsensitive);
}

auto StreamTrack::typeDescription(StreamType type, bool albumart) -> QString
{
    switch (type) {
    case StreamVideo:
        if (albumart)
            return tr("Album Art");
        return tr("Video Track");
    case StreamAudio:
        return tr("Audio Track");
    case StreamSubtitle:
    case StreamInclusiveSubtitle:
        return tr("Subtitle Track");
    default:
        return QString();
    }
}

auto StreamTrack::name() const -> QString
{
    QString name = m_title;
    const QString lang = m_displayLang.isEmpty() ? m_lang : m_displayLang;
    if (!lang.isEmpty())
        name += name.isEmpty() ? lang : " ("_a % lang % ")"_a;
    name = name.trimmed();
    if (name.isEmpty())
        name = typeDescription(m_type, m_albumart) % ' '_q % _N(m_id);
    return name;
}

auto StreamTrack::fromMpvData(const QVariant &mpv) -> StreamTrack
{
    const auto map = mpv.toMap();
    StreamTrack track;
    track.m_type = str2type(map[u"type"_q].toString());
    if (track.m_type == StreamUnknown)
        return StreamTrack();
    track.m_albumart = map[u"albumart"_q].toBool();
    track.m_codec = map[u"codec"_q].toString();
    track.m_default = map[u"default"_q].toBool();
    track.m_id = map[u"id"_q].toInt();
    track.m_lang = map[u"lang"_q].toString();
    if (_InRange(2, track.m_lang.size(), 3) && _IsAlphabet(track.m_lang))
        track.m_displayLang = Locale::isoToNativeName(track.m_lang);
    track.m_title = map[u"title"_q].toString();
    track.m_file = map[u"external-filename"_q].toString();
    if (!track.m_file.isEmpty()) {
        if (track.m_file.contains("googlevideo.com/videoplayback"_a))
            track.m_title.clear();
        else
            track.m_title = QFileInfo(track.m_file).fileName();
    }
    track.m_selected = map[u"selected"_q].toBool();
    return track;
}

auto StreamTrack::fromSubComp(const SubComp &comp) -> StreamTrack
{
    StreamTrack track;
    track.m_type = StreamInclusiveSubtitle;
    track.m_id = comp.id();
    track.m_lang = comp.language();
    track.m_selected = comp.selection();
    track.m_file = comp.path();
    if (!track.m_file.isEmpty())
        track.m_title = QFileInfo(track.m_file).fileName();
    track.m_encoding = comp.encoding();
    track.m_fpsBased = comp.isBasedOnFrame();
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

auto StreamTrack::toJson() const -> QJsonObject
{
    if (m_type == StreamUnknown)
        return QJsonObject();
    QJsonObject json;
    json.insert(u"type"_q, type2str(m_type));
    json.insert(u"id"_q, m_id);
    json.insert(u"title"_q, m_title);
    json.insert(u"lang"_q, m_lang);
    json.insert(u"file"_q, m_file);
    json.insert(u"codec"_q, m_codec);
    json.insert(u"displayLang"_q, m_displayLang);
    json.insert(u"selected"_q, m_selected);
    json.insert(u"default"_q, m_default);
    json.insert(u"albumart"_q, m_albumart);
    if (m_type == StreamInclusiveSubtitle || m_type == StreamSubtitle) {
        json.insert(u"fpsBased"_q, m_fpsBased);
        json.insert(u"encoding"_q, m_encoding.toJson());
    }
    return json;
}

auto StreamTrack::setFromJson(const QJsonObject &json) -> bool
{
    StreamTrack track;
    auto type = str2type(json[u"type"_q].toString());
    if (type == StreamUnknown)
        return false;
    m_type = type;
    m_id = json[u"id"_q].toInt(-1);
    m_title = json[u"title"_q].toString();
    m_lang = json[u"lang"_q].toString();
    m_file = json[u"file"_q].toString();
    m_codec = json[u"codec"_q].toString();
    m_displayLang = json[u"displayLang"_q].toString();
    m_selected = json[u"selected"_q].toBool();
    m_default = json[u"default"_q].toBool();
    m_albumart = json[u"albumart"_q].toBool();
    if (type == StreamInclusiveSubtitle || type == StreamSubtitle) {
        m_fpsBased = json[u"fpsBased"_q].toBool();
        m_encoding.setFromJson(json[u"encoding"_q]);
    }
    return true;
}

auto StreamTrack::fromJson(const QJsonObject &json) -> StreamTrack
{
    StreamTrack track;
    track.setFromJson(json);
    return track;
}

/******************************************************************************/

auto StreamList::fileIds() const -> QList<int>
{
    QList<int> ids;
    for (auto &track : m_tracks) {
        if (track.isExternal())
            ids.push_front(track.id());
    }
    return ids;
}
auto StreamList::deselect(int id) -> bool
{
    if (id < 0) {
        bool ret = false;
        for (auto &track : m_tracks) {
            if (track.isSelected()) {
                ret = true;
                track.m_selected = false;
            }
        }
        return ret;
    }
    auto t = track(id);
    if (!t || !t->isSelected())
        return false;
    t->m_selected = false;
    return true;
}
auto StreamList::select(int id) -> bool
{
    auto t = track(id);
    if (!t || t->isSelected())
        return false;
    if (t->isExclusive()) {
        for (auto &track : m_tracks) {
            if (track.isExclusive())
                track.m_selected = false;
        }
    }
    t->m_selected = true;
    return true;
}

auto StreamList::selection() const -> const StreamTrack*
{
    for (int i =  m_tracks.size() - 1; i >= 0; --i) {
        if (m_tracks[i].isSelected())
            return &m_tracks[i];
    }
    return nullptr;
}

auto StreamList::track(int id) -> StreamTrack*
{
    for (auto &track : m_tracks) {
        if (track.id() == id)
            return &track;
    }
    return nullptr;
}

auto StreamList::insert(const StreamTrack &track) -> void
{
    Q_ASSERT(track.type() == m_type);
    for (auto &t : m_tracks) {
        if (t.id() == track.id()) {
            t = track;
            return;
        }
    }
    m_tracks.push_back(track);
}

auto StreamList::toJson() const -> QJsonObject
{
    QJsonArray array;
    for (auto &track : m_tracks)
        array.push_back(track.toJson());
    QJsonObject json;
    json.insert(u"type"_q, type2str(StreamType(m_type)));
    json.insert(u"tracks"_q, array);
    return json;
}

auto StreamList::setFromJson(const QJsonObject &json) -> bool
{
    const auto type = str2type(json[u"type"_q].toString());
    if (type == StreamUnknown) {
        *this = StreamList();
        return true;
    }
    auto it = json.find(u"tracks"_q);
    if (it == json.end() || !it.value().isArray())
        return false;
    m_type = type;
    m_tracks.clear();
    const auto array = it.value().toArray();
    for (auto one : array)
        insert(StreamTrack::fromJson(one.toObject()));
    return true;
}

#include "channelmanipulation.hpp"
#include "channellayoutmap.hpp"
#include "misc/json.hpp"

static auto nameToId(const QString &name) -> mp_speaker_id {
    const int size = ChannelLayoutMap::channelNames().size();
    for (int i=0; i<size; ++i) {
        if (name == _L(ChannelLayoutMap::channelNames()[i].abbr))
            return (mp_speaker_id)i;
    }
    return MP_SPEAKER_ID_COUNT;
}

template<>
struct JsonIO<mp_speaker_id> {
    SCA qt_type = QJsonValue::String;
    auto toJson(mp_speaker_id id) const noexcept -> QJsonValue
    { return _L(ChannelLayoutMap::channelNames()[id].abbr); }
    auto fromJson(mp_speaker_id &id, const QJsonValue &json) const -> bool
    {
        const auto speaker = nameToId(json.toString());
        if (speaker == MP_SPEAKER_ID_COUNT)
            return false;
        id = speaker;
        return true;
    }
};

auto ChannelManipulation::isIdentity() const -> bool
{
    for (int i=0; i<m_mix.size(); ++i) {
        const auto speaker_out = (mp_speaker_id)i;
        auto &sources = this->sources(speaker_out);
        if (sources.size() != 1 || sources.first() == speaker_out)
            return false;
    }
    return true;
}

auto ChannelManipulation::toJson() const -> QJsonArray
{
    auto obj = json_io(&m_mix)->toJson(m_mix);
    return obj;
}

auto ChannelManipulation::setFromJson(const QJsonArray &json) -> bool
{
    ChannelManipulation man;
    if (!json_io(&m_mix)->fromJson(man.m_mix, json))
        return false;
    m_mix = man.m_mix;
    return true;
}

auto ChannelManipulation::toString() const -> QString
{
    QStringList list;
    for (int i=0; i<(int)m_mix.size(); ++i) {
        auto speaker = (mp_speaker_id)i;
        if (_InRange(MP_SPEAKER_ID_FL, speaker, MP_SPEAKER_ID_SR)
                && !m_mix[i].isEmpty()) {
            QStringList srcs;
            for (auto &src : m_mix[i])
                srcs.push_back(_L(ChannelLayoutMap::channelNames()[src].abbr));
            list.push_back(_L(ChannelLayoutMap::channelNames()[speaker].abbr)
                           % '!'_q % srcs.join('/'_q));
        }
    }
    return list.join(','_q);
}

auto ChannelManipulation::fromString(const QString &text) -> ChannelManipulation
{
    ChannelManipulation man;
    auto list = text.split(','_q);

    for (auto &one : list) {
        auto map = one.split('!'_q, QString::SkipEmptyParts);
        if (map.size() != 2)
            continue;
        auto dest = nameToId(map[0]);
        if (dest == MP_SPEAKER_ID_COUNT)
            continue;
        auto srcs = map[1].split('/'_q, QString::SkipEmptyParts);
        SourceArray sources;
        for (int i=0; i<srcs.size(); ++i) {
            auto src = nameToId(srcs[i]);
            if (src != MP_SPEAKER_ID_COUNT)
                sources.append(src);
        }
        if (!sources.isEmpty())
            man.set(dest, sources);
    }
    return man;
}

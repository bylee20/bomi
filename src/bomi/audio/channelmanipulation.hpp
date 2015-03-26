#ifndef CHANNELMANIPULATION_HPP
#define CHANNELMANIPULATION_HPP

#include "global.hpp"

extern "C" {
#include <audio/chmap.h>
}

#ifdef bool
#undef bool
#endif

enum class ChannelLayout;               enum class SpeakerId;

class ChannelManipulation {
public:
    ChannelManipulation(): m_mix(MP_SPEAKER_ID_COUNT) {}
    using SourceArray = QVector<mp_speaker_id>;
    DECL_EQ(ChannelManipulation, &T::m_mix)
    auto sources(int speaker_out) const -> const SourceArray&
        { return m_mix[speaker_out]; }
    auto hasSources(mp_speaker_id dest) const -> bool
        { return !m_mix[dest].isEmpty(); }
    auto toString() const -> QString;
    static auto fromString(const QString &text) -> ChannelManipulation;
    auto toJsonObject() const -> QJsonObject;
    auto toJsonArray() const -> QJsonArray;
    auto setFromJsonObject(const QJsonObject &json) -> bool;
    auto setFromJsonArray(const QJsonArray &json) -> bool;
private:
    auto set(mp_speaker_id dest, const SourceArray &src) -> void
        { m_mix[dest] = src; }
    auto set(mp_speaker_id dest, mp_speaker_id src) -> void
        { m_mix[dest].resize(1); m_mix[dest][0] = src; }
    auto add(mp_speaker_id dest, mp_speaker_id src) -> void
        { m_mix[dest].append(src); }
    friend class ChannelLayoutMap;
    friend class ChannelManipulationWidget;
    QVector<SourceArray> m_mix;
};

#endif // CHANNELMANIPULATION_HPP

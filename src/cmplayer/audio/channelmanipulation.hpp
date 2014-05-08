#ifndef CHANNELMANIPULATION_HPP
#define CHANNELMANIPULATION_HPP

#include "stdafx.hpp"
extern "C" {
#include <audio/chmap.h>
}

enum class ChannelLayout;               enum class SpeakerId;

class ChannelManipulation {
public:
    ChannelManipulation(): m_mix(MP_SPEAKER_ID_COUNT) {}
    using SourceArray = QVector<mp_speaker_id>;
    auto sources(int speaker_out) const -> const SourceArray&
        { return m_mix[speaker_out]; }
    auto hasSources(mp_speaker_id dest) const -> bool
        { return !m_mix[dest].isEmpty(); }
    auto toString() const -> QString;
    auto isIdentity() const -> bool;
    static auto fromString(const QString &text) -> ChannelManipulation;
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

class ChannelLayoutMap {
public:
    auto operator () (ChannelLayout src,
                      ChannelLayout dest) const -> ChannelManipulation
        { return m_map[src][dest]; }
    auto operator () (const mp_chmap &src,
                      const mp_chmap &dest) const -> ChannelManipulation
        { return m_map[toLayout(src)][toLayout(dest)]; }
    auto toString() const -> QString;
    auto isEmpty() const -> bool { return m_map.isEmpty(); }
    static auto toLayout(const mp_chmap &chmap) -> ChannelLayout;
    static auto fromString(const QString &text) -> ChannelLayoutMap;
    static auto default_() -> ChannelLayoutMap;
private:
    auto get(ChannelLayout src, ChannelLayout dest) -> ChannelManipulation&
        { return m_map[src][dest]; }
    QMap<ChannelLayout, QMap<ChannelLayout, ChannelManipulation>> m_map;
    friend class ChannelManipulationWidget;
};

class ChannelManipulationWidget : public QWidget {
    Q_OBJECT
public:
    ChannelManipulationWidget(QWidget *parent = nullptr);
    ~ChannelManipulationWidget();
    auto setMap(const ChannelLayoutMap &map) -> void;
    auto map() const -> ChannelLayoutMap;
    auto setCurrentLayouts(ChannelLayout src, ChannelLayout dst) -> void;
private:
    struct Data;
    Data *d;
};

auto _ChmapFromLayout(mp_chmap *chmap, ChannelLayout layout) -> bool;

#endif // CHANNELMANIPULATION_HPP

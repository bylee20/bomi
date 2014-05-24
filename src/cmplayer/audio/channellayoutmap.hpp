#ifndef CHANNELLAYOUTMAP_HPP
#define CHANNELLAYOUTMAP_HPP

#include "stdafx.hpp"
#include "channelmanipulation.hpp"

struct ChannelName {const char *abbr; const char *desc;};

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
    static auto channelNames() -> const QVector<ChannelName>&;
private:
    auto get(ChannelLayout src, ChannelLayout dest) -> ChannelManipulation&
        { return m_map[src][dest]; }
    QMap<ChannelLayout, QMap<ChannelLayout, ChannelManipulation>> m_map;
    friend class ChannelManipulationWidget;
};

auto _ChmapFromLayout(mp_chmap *chmap, ChannelLayout layout) -> bool;

#endif // CHANNELLAYOUTMAP_HPP

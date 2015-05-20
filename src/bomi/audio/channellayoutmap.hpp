#ifndef CHANNELLAYOUTMAP_HPP
#define CHANNELLAYOUTMAP_HPP

#include "channelmanipulation.hpp"

struct ChannelName {
    ChannelName(const char *abbr, const char *desc)
        : abbr(abbr), desc(desc) { }
    const char *abbr;
    auto description() const -> QString;
private:
    friend class ChannelLayoutMap;
    const char *desc;
};

class ChannelLayoutMap {
public:
    ChannelLayoutMap();
    DECL_EQ(ChannelLayoutMap, &T::m_map)
    auto operator () (ChannelLayout src,
                      ChannelLayout dest) const -> ChannelManipulation
        { return m_map[src][dest]; }
    auto operator () (const mp_chmap &src,
                      const mp_chmap &dest) const -> ChannelManipulation
        { return m_map[toLayout(src)][toLayout(dest)]; }
    auto toString() const -> QString;
    auto isEmpty() const -> bool { return m_map.isEmpty(); }
    auto isIdentity(const mp_chmap &src, const mp_chmap &dest) const -> bool;
    static auto toLayout(const mp_chmap &chmap) -> ChannelLayout;
    static auto fromString(const QString &text) -> ChannelLayoutMap;
    static auto default_() -> ChannelLayoutMap;
    static auto channelNames() -> const QVector<ChannelName>&;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
private:
    auto get(ChannelLayout src, ChannelLayout dest) -> ChannelManipulation&
        { return m_map[src][dest]; }
    auto get(ChannelLayout src, ChannelLayout dest) const -> ChannelManipulation
        { return m_map[src][dest]; }
    QMap<ChannelLayout, QMap<ChannelLayout, ChannelManipulation>> m_map;
    friend class ChannelManipulationWidget;
};

auto _ChmapFromLayout(mp_chmap *chmap, ChannelLayout layout) -> bool;
auto _ChmapNameFromLayout(ChannelLayout layout) -> QByteArray;

Q_DECLARE_METATYPE(ChannelLayoutMap)

#endif // CHANNELLAYOUTMAP_HPP

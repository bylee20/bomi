#include "channellayoutmap.hpp"
#include "misc/log.hpp"
#include "enum/channellayout.hpp"

DECLARE_LOG_CONTEXT(Audio)

auto _ChmapNameFromLayout(ChannelLayout layout) -> QByteArray
{
    auto name = ChannelLayoutInfo::data(layout);
    switch (layout) {
#ifdef Q_OS_LINUX
    case ChannelLayout::_5_0:
    case ChannelLayout::_4_1:
    case ChannelLayout::_5_1:
    case ChannelLayout::_7_1:
        return name + "(alsa)";
#endif
    default:
        return name;
    }
}

auto _ChmapFromLayout(mp_chmap *chmap, ChannelLayout layout) -> bool
{
    const auto data = _ChmapNameFromLayout(layout);
    return mp_chmap_from_str(chmap, bstr0(data.constData()));
}

SIA to_mp_speaker_id(SpeakerId speaker) -> mp_speaker_id
    { return SpeakerIdInfo::data(speaker); }

static auto speakersInLayout(ChannelLayout layout) -> QVector<SpeakerId>
{
    QVector<SpeakerId> list;
#define CHECK(a) {if (SpeakerId::a & layout) {list << SpeakerId::a;}}
    CHECK(FrontLeft);
    CHECK(FrontRight);
    CHECK(FrontCenter);
    CHECK(LowFrequency);
    CHECK(BackLeft);
    CHECK(BackRight);
    CHECK(FrontLeftCenter);
    CHECK(FrontRightCenter);
    CHECK(BackCenter);
    CHECK(SideLeft);
    CHECK(SideRight);
#undef CHECK
    return list;
}

ChannelLayoutMap::ChannelLayoutMap()
{
    static const auto map = [] () {
        QMap<ChannelLayout, QMap<ChannelLayout, ChannelManipulation>> map;
        auto &items = ChannelLayoutInfo::items();
        auto _mp = [] (SpeakerId speaker) { return to_mp_speaker_id(speaker); };

        for (auto &srcItem : items) {
            const auto srcLayout = srcItem.value;
            const auto srcSpeakers = speakersInLayout(srcLayout);
            for (auto &dstItem : items) {
                const auto dstLayout = dstItem.value;
                auto &mix = map[srcLayout][dstLayout].m_mix;
                for (auto srcSpeaker : srcSpeakers) {
                    const auto mps = to_mp_speaker_id(srcSpeaker);
                    if (srcSpeaker & dstLayout) {
                        mix[mps].push_back(mps);
                        continue;
                    }
                    if (dstLayout == ChannelLayout::Mono) {
                        mix[MP_SPEAKER_ID_FC] << mps;
                        continue;
                    }

                    auto testAndSet = [&] (SpeakerId id) {
                        if (dstLayout & id) {
                            mix[_mp(id)].push_back(mps);
                            return true;
                        } else
                            return false;
                    };
                    auto testAndSet2 = [&] (SpeakerId left, SpeakerId right) {
                        if (dstLayout & (left | right)) {
                            mix[_mp(left)].push_back(mps);
                            mix[_mp(right)].push_back(mps);
                            return true;
                        } else
                            return false;
                    };
                    auto setLeft = [&] () { mix[MP_SPEAKER_ID_FL].push_back(mps); };
                    auto setRight = [&] () { mix[MP_SPEAKER_ID_FR].append(mps); };
                    auto setBoth = [&] { setLeft(); setRight(); };
                    switch (srcSpeaker) {
                    case SpeakerId::FrontLeft:
                    case SpeakerId::FrontRight:
                        Q_ASSERT(false);
                        break;
                    case SpeakerId::LowFrequency:
                        if (!testAndSet(SpeakerId::FrontCenter)
                                || !testAndSet(SpeakerId::FrontLeftCenter))
                            setLeft();
                        break;
                    case SpeakerId::FrontCenter:
                        if (!testAndSet2(SpeakerId::FrontLeftCenter,
                                         SpeakerId::FrontRightCenter))
                            setBoth();
                        break;
                    case SpeakerId::BackLeft:
                        if (testAndSet(SpeakerId::BackCenter)
                                || testAndSet(SpeakerId::SideLeft))
                            break;
                    case SpeakerId::FrontLeftCenter:
                        setLeft();
                        break;
                    case SpeakerId::BackRight:
                        if (testAndSet(SpeakerId::BackCenter)
                                || testAndSet(SpeakerId::SideRight))
                            break;
                    case SpeakerId::FrontRightCenter:
                        setRight();
                        break;
                    case SpeakerId::SideRight:
                        if (!testAndSet(SpeakerId::BackRight))
                            setRight();
                        break;
                    case SpeakerId::SideLeft:
                        if (!testAndSet(SpeakerId::BackLeft))
                            setLeft();
                        break;
                    case SpeakerId::BackCenter:
                        if (!testAndSet2(SpeakerId::BackLeft,
                                         SpeakerId::BackRight)
                                && !testAndSet2(SpeakerId::SideLeft,
                                                SpeakerId::SideRight))
                            setBoth();
                        break;
                    }
                }
            }
        }
        return map;
    }();
    m_map = map;
}

auto ChannelLayoutMap::isIdentity(const mp_chmap &src, const mp_chmap &dest) const -> bool
{
    const auto ls = toLayout(src);
    const auto ld = toLayout(dest);
    if (ls != ld)
        return false;
    auto man = m_map[ls][ld];
    for (int i = 0; i < dest.num; ++i) {
        const auto out = (mp_speaker_id)dest.speaker[i];
        if (!man.hasSources(out))
            return false;
        const auto &s = man.sources(out);
        if (s.size() != 1)
            return false;
        if (s.first() != out)
            return false;
    }
    return true;
}

auto ChannelLayoutMap::default_() -> ChannelLayoutMap
{
    return ChannelLayoutMap();
}

auto ChannelLayoutMap::toLayout(const mp_chmap &chmap) -> ChannelLayout
{
    auto &items = SpeakerIdInfo::items();
    int layout = 0;
    for (int i=0; i<chmap.num; ++i) {
        auto id = (SpeakerId)(-1);
        for (auto &item : items) {
            if (item.data == chmap.speaker[i]) {
                id = item.value;
                break;
            }
        }
        if (id < 0) {
            char str[64] = {0};
            _Error("Cannot convert mp_chmap(%%) to ChannelLayout",
                   mp_chmap_to_str_buf(str, 64, &chmap));
            talloc_free(str);
            return ChannelLayoutInfo::default_();
        }
        layout |= static_cast<int>(id);
    }
    return ChannelLayoutInfo::from(layout);
}

auto ChannelLayoutMap::toString() const -> QString
{
    QStringList list;
    for (auto sit = m_map.begin(); sit != m_map.end(); ++sit) {
        auto srcName = ChannelLayoutInfo::name(sit.key());
        for (auto dit = sit->begin(); dit != sit->end(); ++dit) {
            auto dstName = ChannelLayoutInfo::name(dit.key());
            list.push_back(srcName % ':'_q % dstName % ':'_q % dit->toString());
        }
    }
    return list.join('#'_q);
}

auto ChannelLayoutMap::fromString(const QString &text) -> ChannelLayoutMap
{
    auto list = text.split('#'_q, QString::SkipEmptyParts);
    ChannelLayoutMap map;
    for (auto &one : list) {
        auto parts = one.split(':'_q, QString::SkipEmptyParts);
        if (parts.size() != 3)
            continue;
        auto src = ChannelLayoutInfo::from(parts[0]);
        auto dst = ChannelLayoutInfo::from(parts[1]);
        auto man = ChannelManipulation::fromString(parts[2]);
        map.get(src, dst) = man;
    }
    return map;
}

auto ChannelName::description() const -> QString
{
    return qApp->translate("SpeakerId", desc);
}

auto ChannelLayoutMap::channelNames() -> const QVector<ChannelName>&
{
    static const QVector<ChannelName> ChNames = {
        { "FL",  QT_TRANSLATE_NOOP("SpeakerId", "Front Left") },
        { "FR",  QT_TRANSLATE_NOOP("SpeakerId", "Front Right") },
        { "FC",  QT_TRANSLATE_NOOP("SpeakerId", "Front Center") },
        { "LFE", QT_TRANSLATE_NOOP("SpeakerId", "Low Frequency Effects") },
        { "BL",  QT_TRANSLATE_NOOP("SpeakerId", "Back Left") },
        { "BR",  QT_TRANSLATE_NOOP("SpeakerId", "Back Right") },
        { "FLC", QT_TRANSLATE_NOOP("SpeakerId", "Front Left-of-Center") },
        { "FRC", QT_TRANSLATE_NOOP("SpeakerId", "Front Right-of-Center") },
        { "BC",  QT_TRANSLATE_NOOP("SpeakerId", "Back Center") },
        { "SL",  QT_TRANSLATE_NOOP("SpeakerId", "Side Left") },
        { "SR",  QT_TRANSLATE_NOOP("SpeakerId", "Side Right") }
    };
    return ChNames;
}

#include "misc/json.hpp"

auto ChannelLayoutMap::toJson() const -> QJsonObject
{
    QJsonObject json;
    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        QJsonObject obj;
        for (auto iit = it->begin(); iit != it->end(); ++iit)
            obj.insert(_EnumName(iit.key()), _JsonToString(iit->toJsonObject()));
        json.insert(_EnumName(it.key()), obj);
    }
    return json;
}

auto ChannelLayoutMap::setFromJson(const QJsonObject &json) -> bool
{
    ChannelLayoutMap other; auto &map = other.m_map;
    bool ret = true;
    for (auto it = map.begin(); it != map.end(); ++it) {
        auto obj = json.value(_EnumName(it.key())).toObject();
        if (obj.isEmpty())
            continue;
        for (auto iit = it->begin(); iit != it->end(); ++iit) {
            auto value = obj.value(_EnumName(iit.key()));
            if (value.isArray())
                ret &= iit->setFromJsonArray(value.toArray());
            else if (value.isString())
                ret &= iit->setFromJsonObject(_JsonFromString(value.toString()));
            else
                ret = false;
        }
    }
    m_map = other.m_map;
    return ret;
}

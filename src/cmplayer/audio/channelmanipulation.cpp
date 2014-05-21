#include "channelmanipulation.hpp"
#include "log.hpp"
#include "record.hpp"

DECLARE_LOG_CONTEXT(Audio)

static const QVector<ChannelName> ChNames = {
    { "FL", "Front Left" },
    { "FR", "Front Right" },
    { "FC", "Front Center" },
    { "LFE", "Low Frequency Effects" },
    { "BL", "Back Left" },
    { "BR", "Back Right" },
    { "FLC", "Front Left-of-Center" },
    { "FRC", "Front Right-of-Center" },
    { "BC", "Back Center" },
    { "SL", "Side Left" },
    { "SR", "Side Right" }
};

static const int ChNamesSize = ChNames.size();

auto _ChmapFromLayout(mp_chmap *chmap, ChannelLayout layout) -> bool
{
    const auto data = ChannelLayoutInfo::data(layout);
    return mp_chmap_from_str(chmap, bstr0(data.constData()));
}

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

auto ChannelManipulation::toString() const -> QString
{
    QStringList list;
    for (int i=0; i<(int)m_mix.size(); ++i) {
        auto speaker = (mp_speaker_id)i;
        if (_InRange(MP_SPEAKER_ID_FL, speaker, MP_SPEAKER_ID_SR)
                && !m_mix[i].isEmpty()) {
            QStringList srcs;
            for (auto &src : m_mix[i])
                srcs.push_back(_L(ChNames[src].abbr));
            list.push_back(_L(ChNames[speaker].abbr) % '!' % srcs.join('/'));
        }
    }
    return list.join(',');
}

auto ChannelManipulation::fromString(const QString &text) -> ChannelManipulation
{
    ChannelManipulation man;
    auto list = text.split(',');
    auto nameToId = [] (const QString &name) {
        for (int i=0; i<ChNamesSize; ++i) {
            if (name == _L(ChNames[i].abbr))
                return (mp_speaker_id)i;
        }
        return MP_SPEAKER_ID_COUNT;
    };

    for (auto &one : list) {
        auto map = one.split('!', QString::SkipEmptyParts);
        if (map.size() != 2)
            continue;
        auto dest = nameToId(map[0]);
        if (dest == MP_SPEAKER_ID_COUNT)
            continue;
        auto srcs = map[1].split('/', QString::SkipEmptyParts);
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

static inline auto to_mp_speaker_id(SpeakerId speaker) -> mp_speaker_id
    { return SpeakerIdInfo::data(speaker); }

static auto speakersInLayout(ChannelLayout layout) -> QVector<SpeakerId>
{
    QVector<SpeakerId> list;
#define CHECK(a) {if (SpeakerId::a & (int)layout) {list << SpeakerId::a;}}
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

auto ChannelLayoutMap::default_() -> ChannelLayoutMap
{
    ChannelLayoutMap map;
    auto &items = ChannelLayoutInfo::items();
    auto _mp = [] (SpeakerId speaker) { return to_mp_speaker_id(speaker); };

    for (auto &srcItem : items) {
        const auto srcLayout = srcItem.value;
        const auto srcSpeakers = speakersInLayout(srcLayout);
        for (auto &dstItem : items) {
            const auto dstLayout = dstItem.value;
            auto &mix = map.get(srcLayout, dstLayout).m_mix;
            for (auto srcSpeaker : srcSpeakers) {
                const auto mps = to_mp_speaker_id(srcSpeaker);
                if (srcSpeaker & (int)dstLayout) {
                    mix[mps].push_back(mps);
                    continue;
                }
                if (dstLayout == ChannelLayout::Mono) {
                    mix[MP_SPEAKER_ID_FC] << mps;
                    continue;
                }

                auto testAndSet = [&] (SpeakerId id) {
                    if (dstLayout & (int)id) {
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
            char *str = mp_chmap_to_str(&chmap);
            _Error("Cannot convert mp_chmap(%%) to ChannelLayout", str);
            talloc_free(str);
            return ChannelLayoutInfo::default_();
        }
        layout |= id;
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
            list.push_back(srcName % ":" % dstName % ":" % dit->toString());
        }
    }
    return list.join('#');
}

auto ChannelLayoutMap::fromString(const QString &text) -> ChannelLayoutMap
{
    auto list = text.split('#', QString::SkipEmptyParts);
    ChannelLayoutMap map;
    for (auto &one : list) {
        auto parts = one.split(':', QString::SkipEmptyParts);
        if (parts.size() != 3)
            continue;
        auto src = ChannelLayoutInfo::from(parts[0]);
        auto dst = ChannelLayoutInfo::from(parts[1]);
        auto man = ChannelManipulation::fromString(parts[2]);
        map.get(src, dst) = man;
    }
    return map;
}

auto ChannelLayoutMap::channelNames() -> const QVector<ChannelName>&
{
    return ChNames;
}

/*****************************************************/


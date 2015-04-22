#include "mrlstate.hpp"
#include "mrlstatesqlfield.hpp"
#include "mrlstate_old.hpp"
#include "mrlstate_p.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"
#include "misc/log.hpp"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

DECLARE_LOG_CONTEXT(History)

MrlState::MrlState()
    : d(new Data) {
    m_tracks.resize(StreamUnknown);
    m_tracks[StreamVideo] = { &m_video_tracks, &MrlState::video_tracks_changed, &MrlState::desc_video_tracks };
    m_tracks[StreamAudio] = { &m_audio_tracks, &MrlState::audio_tracks_changed, &MrlState::desc_audio_tracks };
    m_tracks[StreamSubtitle] = { &m_sub_tracks, &MrlState::sub_tracks_changed, &MrlState::desc_sub_tracks };
    m_tracks[StreamInclusiveSubtitle] = { &m_sub_tracks_inclusive, &MrlState::sub_tracks_inclusive_changed, &MrlState::desc_sub_tracks };
    for (int i = 0; i < m_tracks.size(); ++i)
        connect(this, m_tracks[i].signal, this, [=] () { emit tracksChanged((StreamType)i); });
}

MrlState::~MrlState()
{
    delete d;
}

auto MrlState::select(StreamType type, int id) -> void
{
    bool locked = m_mutex && m_mutex->tryLock();
    auto tracks = m_tracks[type].tracks;
    if ((id < 0 && tracks->deselect(-1)) || (id >= 0 && tracks->select(id))) {
        emit (this->*m_tracks[type].signal)(*m_tracks[type].tracks);
        emit currentTrackChanged(type);
    }
    if (locked)
        m_mutex->unlock();
}

auto MrlState::toJson() const -> QJsonObject
{
    auto json = _JsonFromQObject(this);
    json.insert(u"version"_q, Version);
    json.insert(u"video_interpolator_down_map"_q, d->intrplDown.toJson());
    json.insert(u"video_interpolator_map"_q, d->intrpl.toJson());
    json.insert(u"video_chroma_upscaler_map"_q, d->chroma.toJson());
    return json;
}

auto MrlState::setFromJson(const QJsonObject &json) -> bool
{
    const int version = json.value(u"version"_q).toInt();
    if (json.value(u"version"_q).toInt() == Version) {
        bool ret = _JsonToQObject(json, this);
        auto set = [&] (const QString &key, IntrplParamSetMap &map) {
            auto it = json.find(key);
            return it != json.end() && map.setFromJson(it.value().toArray());
        };
        ret = set(u"video_interpolator_down_map"_q, d->intrplDown) && ret;
        ret = set(u"video_interpolator_map"_q, d->intrpl) && ret;
        ret = set(u"video_chroma_upscaler_map"_q, d->chroma) && ret;
        return ret;
    } else {
        if (version <= 3) {
            _Info("Importing version %% JSON.", 3);
            MrlStateV3 v3;
            _JsonToQObject(json, &v3);
            import(&v3);
            return true;
        }
    }
    return false;
}

auto MrlState::copyFrom(const MrlState *state) -> void
{
    auto mo = metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        auto p = mo->property(i);
        p.write(this, p.read(state));
    }
    *d = *state->d;
}

auto MrlState::description(const char *property) -> QString
{
    auto mo = &staticMetaObject;
    for (int i = 0; i < mo->classInfoCount(); ++i) {
        const auto info = mo->classInfo(i);
        if (!qstrcmp(info.name(), property))
            return tr(info.value());
    }
    return QString();
}

auto MrlState::notifySignal(const char *property) const -> QMetaMethod
{
    auto p = metaProperty(property);
    return p.notifySignal();
}

auto MrlState::notifyAll() const -> void
{
    auto mo = metaObject();
    for (int i = 1; i < mo->propertyCount(); ++i) {
        auto p = mo->property(i);
        if (p.hasNotifySignal())
            p.notifySignal().invoke(const_cast<MrlState*>(this),
                                    QGenericArgument(p.typeName(),
                                                     p.read(this).constData()));
    }
}

auto MrlState::metaProperty(const char *property) const -> QMetaProperty
{
    auto mo = metaObject();
    const int idx = mo->indexOfProperty(property);
    if (idx < 0)
        return QMetaProperty();
    return mo->property(idx);
}

auto MrlState::defaultProperties() -> QStringList
{
    return { u"name"_q, u"device"_q, u"last_played_date_time"_q,
             u"resume_position"_q, u"edition"_q, u"star"_q };
}

auto MrlState::restorableProperties() -> QVector<PropertyInfo>
{
    auto mo = &staticMetaObject;
    QVector<PropertyInfo> properties;
    properties.reserve(mo->propertyCount());
    PropertyInfo info;
    for (int i = 1; i < mo->propertyCount(); ++i) {
        info.property = _L(mo->property(i).name());
        info.description = description(mo->property(i).name());
        if (!info.description.isEmpty())
            properties.push_back(info);
    }
    return properties;
}

auto MrlState::import(const MrlStateV3 *v3) -> void
{
    if (m_mutex)
        m_mutex->lock();
#define COPY(var) {static_assert(tmp::is_same<decltype(var()), decltype(v3->var())>(), "!!!"); set_##var(v3->var());}
    COPY(mrl);
    COPY(device);
    COPY(last_played_date_time);
    COPY(resume_position);

    COPY(edition);
    set_play_speed(v3->play_speed() * 1e-2);

    COPY(video_interpolator);
    COPY(video_chroma_upscaler);
    set_video_aspect_ratio(_EnumData(v3->video_aspect_ratio()));
    set_video_crop_ratio(_EnumData(v3->video_crop_ratio()));
    COPY(video_deinterlacing);
    COPY(video_dithering);
    set_video_offset(QPointF(v3->video_offset()) * 1e-2);
    COPY(video_vertical_alignment);
    COPY(video_horizontal_alignment);
    COPY(video_color);
    COPY(video_range);
    COPY(video_space);
    COPY(video_hq_upscaling);
    COPY(video_hq_downscaling);
    COPY(video_motion_interpolation);
    COPY(video_effects);
    COPY(video_tracks);

    set_audio_volume(v3->audio_volume() * 1e-2);
    set_audio_amplifier(v3->audio_amplifier() * 1e-2);
    COPY(audio_equalizer);
    COPY(audio_sync);
    COPY(audio_tracks);
    COPY(audio_muted);
    COPY(audio_volume_normalizer);
    COPY(audio_tempo_scaler);
    COPY(audio_channel_layout);

    COPY(sub_alignment);
    COPY(sub_display);
    set_sub_position(v3->sub_position() * 1e-2);
    COPY(sub_sync);
    COPY(sub_tracks);
    COPY(sub_tracks_inclusive);
    COPY(sub_hidden);
    COPY(sub_style_overriden);

    d->intrpl = v3->video_interpolator_map();
    d->chroma = v3->video_chroma_upscaler_map();

    if (m_mutex)
            m_mutex->unlock();
}

auto _ImportMrlStates(int version, QSqlDatabase db)
-> QVector<MrlState*>
{
    Q_UNUSED(db);
    QVector<MrlState*> states;
    if (version < 3)
        _Error("This version of history database is not supported.");
    else if (version == 3) {
        MrlStateV3 v3;
        _Info("Importing from %%.", v3.table());
        QSqlQuery query(db);
        if (!query.exec("SELECT * FROM "_a % v3.table())) {
            _Error("Failed to execute query to import.");
            _Error("Query: %%", query.lastQuery());
            _Error("Error: %%", query.lastError().text());
            return states;
        }
        const auto record = query.record();
        QVector<MrlStateSqlField> fields(record.count());
        for (int i = 0; i < record.count(); ++i) {
            const auto mo = v3.metaObject();
            const int idx = mo->indexOfProperty(record.fieldName(i).toLatin1());
            if (idx < 0) {
                _Warn("Field %% is not a property of old MrlState.", record.fieldName(i));
                continue;
            }
            const auto p = mo->property(idx);
            fields[i] = MrlStateSqlField(p, p.read(&v3));
        }

        while (query.next()) {
            MrlStateV3 v3;
            for (int i = 0; i < record.count(); ++i) {
                if (fields[i].isValid())
                    fields[i].exportTo(&v3, query.value(i));
            }
            auto state = new MrlState;
            state->import(&v3);
            states.push_back(state);
        }
        _Info("%% records imported.", states.size());
    }
    return states;
}

#include "mrlstate.hpp"
#include "mrlstate_p.hpp"
#include "misc/json.hpp"
#include "misc/jsonstorage.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(History)

MrlState::MrlState()
    : d(new Data) {
    m_tracks.resize(StreamUnknown);
    m_tracks[StreamVideo] = { &m_video_tracks, &MrlState::video_tracks_changed };
    m_tracks[StreamAudio] = { &m_audio_tracks, &MrlState::audio_tracks_changed };
    m_tracks[StreamSubtitle] = { &m_sub_tracks, &MrlState::sub_tracks_changed };
    m_tracks[StreamInclusiveSubtitle] = { &m_sub_tracks_inclusive, &MrlState::sub_tracks_inclusive_changed };
}

MrlState::~MrlState()
{
    delete d;
}

auto MrlState::select(StreamType type, int id) -> void
{
    bool locked = m_mutex && m_mutex->tryLock();
    if (m_tracks[type].tracks->select(id))
        emit (this->*m_tracks[type].signal)(*m_tracks[type].tracks);
    if (locked)
        m_mutex->unlock();
}

auto MrlState::toJson() const -> QJsonObject
{
    auto json = _JsonFromQObject(this);
    json.insert(u"video_interpolator_map"_q, _ToJson(d->intrpl));
    json.insert(u"video_chroma_upscaler_map"_q, _ToJson(d->chroma));
    return json;
}

auto MrlState::setFromJson(const QJsonObject &json) -> bool
{
    bool ret = _JsonToQObject(json, this);
    auto set = [&] (const QString &key, IntrplParamSetMap &map) {
        auto it = json.find(key);
        if (it == json.end())
            return false;
        map = _FromJson<IntrplParamSetMap>(it.value());
        for (auto &item : InterpolatorInfo::items()) {
            if (!map.contains(item.value))
                map[item.value] = IntrplParamSet::default_(item.value);
        }
        return true;
    };
    ret = set(u"video_interpolator_map"_q, d->intrpl) && ret;
    ret = set(u"video_chroma_upscaler_map"_q, d->chroma) && ret;
    return ret;
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

auto MrlState::description(const char *property) const -> QString
{
    QByteArray name = "desc_";
    name += property;
    name += "()";
    auto mo = metaObject();
    const int idx = mo->indexOfMethod(name.constData());
    if (idx < 0)
        return QString();
    QString ret;
    auto ok = mo->method(idx).invoke(const_cast<MrlState*>(this),
                                     Q_RETURN_ARG(QString, ret));
    return ok ? ret : QString();
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
    QStringList list;
    list << u"device"_q << u"last_played_date_time"_q
         << u"resume_position"_q << u"edition"_q;
    return list;
}

auto MrlState::restorableProperties() -> QVector<PropertyInfo>
{
    MrlState s;
    auto mo = s.metaObject();
    QVector<PropertyInfo> properties;
    properties.reserve(mo->propertyCount());
    PropertyInfo info;
    for (int i =  1; i < mo->propertyCount(); ++i) {
        info.property = mo->property(i);
        info.description = s.description(info.property.name());
        if (!info.description.isEmpty())
            properties.push_back(info);
    }
    return properties;
}

//template<class F>
//static auto _MrlFieldColumnListString(const QList<F> &list) -> QString
//{
//    QString columns;
//    for (auto &f : list)
//        columns += _L(f.property().name()) % ", "_a;
//    columns.chop(2);
//    return columns;
//}

//template<class T = MrlState, class F>
//auto _FillMrlStateFromRecord(T *state, const QList<F> &fields,
//                             const QSqlRecord &record) -> void {
//    for (int i=0; i<fields.size(); ++i) {
//        const auto &f = fields[i];
//        const QMetaProperty p = f.property();
//        Q_ASSERT(_L(p.name()) == record.fieldName(i));
//        p.write(state, f.fromSql(record.value(i)));
//    }
//}

auto _ImportMrlStates(int version, QSqlDatabase db)
-> QVector<MrlState*>
{
    Q_UNUSED(db);
    QVector<MrlState*> states;
    if (version < 3)
        _Error("This version of history database is not supported.");
    return states;
}

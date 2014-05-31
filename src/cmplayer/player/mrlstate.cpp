#include "mrlstate.hpp"
#include "mrlstate_old.hpp"
#include "misc/json.hpp"

template<>
struct JsonIO<Mrl> : JsonQStringType {
    static auto toJson(const Mrl &mrl) -> QJsonValue { return mrl.toString(); }
    static auto fromJson(Mrl &mrl, const QJsonValue &json) -> bool
    {
        if (!json.isString())
            return false;
        mrl = Mrl::fromString(json.toString());
        return true;
    }
};

template<>
struct JsonIO<QDateTime> {
    static auto toJson(const QDateTime &dt) -> QJsonValue
        { return (double)dt.toMSecsSinceEpoch(); }
    static auto fromJson(QDateTime &dt, const QJsonValue &json) -> bool
    {
        if (!json.isDouble())
            return false;
        dt.setMSecsSinceEpoch(std::llround(json.toDouble()));
        return true;
    }
    SCA qt_type = QJsonValue::Double;
};

#define JSON_CLASS MrlState
static const auto jio = JIO(
    JE(mrl),
    JE(device),
    JE(last_played_date_time),
    JE(resume_position),
    JE(play_speed),
    JE(edition),
    JE(video_aspect_ratio),
    JE(video_crop_ratio),
    JE(video_deinterlacing),
    JE(video_interpolator),
    JE(video_chroma_upscaler),
    JE(video_dithering),
    JE(video_offset),
    JE(video_vertical_alignment),
    JE(video_horizontal_alignment),
    JE(video_color),
    JE(video_range),
    JE(video_effects),
    JE(audio_amplifier),
    JE(audio_volume),
    JE(audio_sync),
    JE(audio_muted),
    JE(audio_volume_normalizer),
    JE(audio_tempo_scaler),
    JE(audio_channel_layout),
    JE(audio_track),
    JE(sub_position),
    JE(sub_sync),
    JE(sub_display),
    JE(sub_alignment),
    JE(sub_track)
);

auto MrlState::toJson() const -> QJsonObject
{
    return jio.toJson(*this);
}

auto MrlState::setFromJson(const QJsonObject &json) -> bool
{
    return jio.fromJson(*this, json);
}

auto MrlState::restorableProperties() -> QVector<PropertyInfo>
{
    QVector<PropertyInfo> properties;
    properties.reserve(staticMetaObject.propertyCount());
    auto add = [&properties] (const char *name, const QString &info) {
        const int index = staticMetaObject.indexOfProperty(name);
        Q_ASSERT(index != -1);
        properties.push_back(PropertyInfo{staticMetaObject.property(index), info});
    };
#define ADD(n, i) add(#n, i)
    ADD(play_speed, qApp->translate("MrlState", "Playback Speed"));

    ADD(video_interpolator, qApp->translate("MrlState", "Video Interpolator"));
    ADD(video_chroma_upscaler,
        qApp->translate("MrlState", "Video Chroma Upscaler"));
    ADD(video_aspect_ratio, qApp->translate("MrlState", "Video Aspect Ratio"));
    ADD(video_crop_ratio, qApp->translate("MrlState", "Video Crop Ratio"));
    ADD(video_deinterlacing,
        qApp->translate("MrlState", "Video Deinterlacing"));
    ADD(video_dithering, qApp->translate("MrlState", "Video Dithering"));
    ADD(video_offset, qApp->translate("MrlState", "Video Screen Position"));
    ADD(video_vertical_alignment,
        qApp->translate("MrlState", "Video Vertical Alignment"));
    ADD(video_horizontal_alignment,
        qApp->translate("MrlState", "Video Horizontal Alignment"));
    ADD(video_color, qApp->translate("MrlState", "Video Color Adjustment"));
    ADD(video_range, qApp->translate("MrlState", "Video Color Range"));

    ADD(audio_volume, qApp->translate("MrlState", "Audio Volume"));
    ADD(audio_amplifier, qApp->translate("MrlState", "Audio Amp"));
    ADD(audio_sync, qApp->translate("MrlState", "Audio Sync"));
    ADD(audio_track, qApp->translate("MrlState", "Audio Track"));
    ADD(audio_muted, qApp->translate("MrlState", "Audio Mute"));
    ADD(audio_volume_normalizer,
        qApp->translate("MrlState", "Audio Volume Normalizer"));
    ADD(audio_tempo_scaler, qApp->translate("MrlState", "Audio Tempo Scaler"));
    ADD(audio_channel_layout,
        qApp->translate("MrlState", "Audio Channel Layout"));

    ADD(sub_track, qApp->translate("MrlState", "Subtitle Track"));
    ADD(sub_alignment, qApp->translate("MrlState", "Subtitle Alignment"));
    ADD(sub_display, qApp->translate("MrlState", "Subtitle Display"));
    ADD(sub_position, qApp->translate("MrlState", "Subtitle Position"));
    ADD(sub_sync, qApp->translate("MrlState", "Subtitle Sync"));

    return properties;
}

template<class F>
static auto _MrlFieldColumnListString(const QList<F> &list) -> QString
{
    QString columns;
    for (auto &f : list)
        columns += f.property().name() % ", "_a;
    columns.chop(2);
    return columns;
}

template<class T = MrlState, class F>
auto _FillMrlStateFromRecord(T *state, const QList<F> &fields,
                             const QSqlRecord &record) -> void {
    for (int i=0; i<fields.size(); ++i) {
        const auto &f = fields[i];
        const QMetaProperty p = f.property();
        Q_ASSERT(p.name() == record.fieldName(i));
        p.write(state, f.fromSql(record.value(i)));
    }
}

auto _ImportMrlStates(int version, QSqlDatabase db)
-> QVector<MrlState*>
{
    QVector<MrlState*> states;
    if (version < 1) {
        QSettings set;
        set.beginGroup("history");
        const int size = set.beginReadArray("list");
        states.reserve(size);
        for (int i=0; i<size; ++i) {
            set.setArrayIndex(i);
            const Mrl mrl = set.value("mrl", QString()).toString();
            if (mrl.isEmpty())
                continue;
            auto state = new MrlState;
            state->mrl = mrl;
            state->last_played_date_time
                    = set.value("date", QDateTime()).toDateTime();
            state->resume_position
                    = set.value("stopped-position", 0).toInt();
            states.append(state);
        }
    } else if (version < 2) {
        QSqlQuery query(db);
        db.transaction();
        const auto fields = MrlFieldV1::list();
        const auto columns = _MrlFieldColumnListString(fields);
        MrlStateV1 prev;
        query.exec(QString("SELECT %1, (SELECT COUNT(*) FROM state) as total "
                           "FROM state").arg(columns));
        while (query.next()) {
            _FillMrlStateFromRecord(&prev, fields, query.record());
            auto state = new MrlState;
            prev.fillCurrentVersion(state);
            states.append(state);
        }
        db.rollback();
    } else if (version < 3) {
        const auto fields = MrlFieldV2::list();
        const QString stateTable = "state"_a % _N(2);
        const QString select = QString::fromLatin1("SELECT %1 FROM %2")
                               .arg(_MrlFieldColumnListString(fields));
        QSqlQuery query(db);
        db.transaction();
        query.exec(select.arg(stateTable));
        while (query.next()) {
            auto state = new MrlState;
            _FillMrlStateFromRecord(state, fields, query.record());
            states.push_back(state);
        }
        db.rollback();
    }
    return states;
}

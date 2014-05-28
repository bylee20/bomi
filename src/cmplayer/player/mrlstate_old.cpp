#include "mrlstate_old.hpp"

enum BitHelper {
    B_MSec = 10,
    B_Sec = 6,
    B_Min = 6,
    B_Hour = 5,
    B_Day = 5,
    B_Month = 4,
    B_Year = 14
};

enum ShiftHelper {
    S_MSec = 0,
    S_Sec = S_MSec + B_MSec,
    S_Min = S_Sec + B_Sec,
    S_Hour = S_Min + B_Min,
    S_Day = S_Hour + B_Hour,
    S_Month = S_Day + B_Day,
    S_Year = S_Month + B_Month
};

SIA _ToSql(const QString &text) -> QString {
    return _L('\'') % QString(text).replace(_L('\''), _L("''")) % _L('\'');
}


SIA _ToSql(const QDateTime &dt) -> QString {
    const auto date = dt.date();
    const auto time = dt.time();
#define CV(v, s) (qint64(v) << S_##s)
    qint64 res = 0x0;
    res |= CV(date.year(), Year);
    res |= CV(date.month(), Month);
    res |= CV(date.day(), Day);
    res |= CV(time.hour(), Hour);
    res |= CV(time.msec(), MSec);
    res |= CV(time.second(), Sec);
    res |= CV(time.minute(), Min);
    return QString::number(res);
#undef CV
}

SIA _DateTimeFromSql(qint64 dt) -> QString {
#define XT(s) ((dt >> S_##s) & ((1 << B_##s)-1))
    const QString q("%1");
    auto pad = [&q] (int v, int n) -> QString { return q.arg(v, n, 10, _L('0')); };
    return pad(XT(Year), 4) % _L('/') % pad(XT(Month), 2) % _L('/') % pad(XT(Day), 2) % _L(' ')
            % pad(XT(Hour), 2) % _L(':') % pad(XT(Min), 2) % _L(':') % pad(XT(Sec), 2);
#undef XT
}

SIA _ToSql(qint32 integer) -> QString { return QString::number(integer); }
SIA _ToSql(qint64 integer) -> QString { return QString::number(integer); }

SIA _ToSql(const QPoint &p) -> QString
{
    return _L('\'') % QString::number(p.x()) % ","
                    % QString::number(p.y()) % _L('\'');
}

SIA _ToSql(const QJsonObject &json) -> QString
{ return _ToSql(_JsonToString(json)); }

SIA _PointFromSql(const QString &str, const QPoint &def) -> QPoint {
    auto index = str.indexOf(',');
    if (index < 0)
        return def;
    bool ok1 = false, ok2 = false;
    QPoint ret{str.midRef(0, index).toInt(&ok1), str.midRef(index+1).toInt(&ok2)};
    return ok1 && ok2 ? ret : def;
}

auto MrlFieldV1::list() -> QList<MrlFieldV1>
{
    static QList<MrlFieldV1> fields;
    if (fields.isEmpty()) {
        auto &metaObject = MrlStateV1::staticMetaObject;
        const int count = metaObject.propertyCount();
        const int offset = metaObject.propertyOffset();
        fields.reserve(count - offset);
        for (int i=offset; i<count; ++i) {
            MrlFieldV1 field;
            field.m_property = metaObject.property(i);
            field.m_type = "INTEGER";
            switch (field.m_property.type()) {
            case QVariant::Int:
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toInt()); };
                break;
            case QVariant::LongLong:
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toLongLong()); };
                break;
            case QVariant::Bool:
                field.m_toSql = [] (const QVariant &var) { return _ToSql((int)var.toBool()); };
                break;
            case QVariant::Point:
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toPoint()); };
                field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return _PointFromSql(var.toString(), QPoint()); };
                break;
            case QVariant::DateTime:
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toDateTime()); };
                field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return _DateTimeFromSql(var.toLongLong()); };
                break;
            case QVariant::String:
                field.m_type = "TEXT";
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toString()); };
                break;
            default: {
                Q_ASSERT(field.m_property.type() == QVariant::UserType);
                const auto type = field.m_property.userType();
                if (_IsEnumTypeId(type)) {
                    field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toInt()); };
                    field.m_fromSql = [] (const QVariant &var, int type) -> QVariant { const int i = var.toInt(); return QVariant(type, &i); };
                } else if (type == qMetaTypeId<VideoColor>()) {
                    field.m_toSql = [] (const QVariant &var) {
                        VideoColor color = var.value<VideoColor>();
#define PACK(p, s) ((0xffu & ((quint32)color.p() + 100u)) << s)
                        return _ToSql(qint64(PACK(brightness, 24) | PACK(contrast, 16) | PACK(saturation, 8) | PACK(hue, 0)));
#undef PACK
                    };
                    field.m_fromSql = [] (const QVariant &var, int) -> QVariant {
                        const auto v = var.toLongLong();
#define UNPACK(s) (((v >> s) & 0xff) - 100)
                        VideoColor c;
                        c.setBrightness(UNPACK(24));
                        c.setContrast(UNPACK(16));
                        c.setSaturation(UNPACK(8));
                        c.setHue(UNPACK(0));
                        return QVariant::fromValue(c);
#undef UNPACK
                    };
                } else if (type == qMetaTypeId<SubtitleStateInfo>()) {
                    field.m_type = "TEXT";
                    field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<SubtitleStateInfo>().toString()); };
                    field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return QVariant::fromValue(SubtitleStateInfo::fromString(var.toString())); };
                } else if (type == qMetaTypeId<Mrl>()) {
                    field.m_type = "TEXT UNIQUE";
                    field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<Mrl>().toString()); };
                    field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return QVariant::fromValue(Mrl::fromString(var.toString())); };
                } else
                    Q_ASSERT_X(false, "HistoryDatabaseModel::HistoryDatabaseModel()", "wrong type!");

            }}
            fields.append(field);
        }
    }
    return fields;
}

template<class T> static inline bool _Is(int type) { return qMetaTypeId<T>() == type; }

auto MrlFieldV2::list() -> QList<MrlFieldV2>
{
    static QList<MrlFieldV2> fields;
    if (!fields.isEmpty())
        return fields;
    MrlState default_;
    auto &metaObject = MrlState::staticMetaObject;
    const int count = metaObject.propertyCount();
    const int offset = metaObject.propertyOffset();
    Q_ASSERT(offset == 1);
    fields.reserve(count - offset);
    for (int i=offset; i<count; ++i) {
        MrlFieldV2 field;
        field.m_property = metaObject.property(i);
        field.m_sqlType = "INTEGER";
        field.m_defaultValue = field.m_property.read(&default_);
        switch (field.m_property.type()) {
        case QVariant::Int:
            field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toInt()); };
            break;
        case QVariant::LongLong:
            field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toLongLong()); };
            break;
        case QVariant::Bool:
            field.m_toSql = [] (const QVariant &var) { return _ToSql((int)var.toBool()); };
            break;
        case QVariant::Point:
            field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toPoint()); };
            field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
                return _PointFromSql(var.toString(), def.toPoint());
            };
            break;
        case QVariant::DateTime:
            field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toDateTime()); };
            field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
                return var.isNull() ? def : _DateTimeFromSql(var.toLongLong());
            };
            break;
        case QVariant::String:
            field.m_sqlType = "TEXT";
            field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toString()); };
            break;
        default: {
            Q_ASSERT(field.m_property.type() == QVariant::UserType);
            const auto type = field.m_property.userType();
            if (_GetEnumFunctionsForSql(type, field.m_toSql, field.m_fromSql)) {
                field.m_sqlType = "TEXT";
            } else if (_Is<VideoColor>(type)) {
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<VideoColor>().packed()); };
                field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
                    return var.isNull() ? def : QVariant::fromValue(VideoColor::fromPacked(var.toLongLong()));
                };
            } else if (_Is<SubtitleStateInfo>(type)) {
                field.m_sqlType = "TEXT";
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<SubtitleStateInfo>().toJson()); };
                field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
                    return var.isNull() ? def : QVariant::fromValue(SubtitleStateInfo::fromJson(var.toString()));
                };
            } else if (_Is<Mrl>(type)) {
                field.m_sqlType = "TEXT PRIMARY KEY NOT NULL";
                field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<Mrl>().toString()); };
                field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
                    return var.isNull() ? def : QVariant::fromValue(Mrl::fromString(var.toString()));
                };
            } else
                Q_ASSERT_X(false, "HistoryDatabaseModel::HistoryDatabaseModel()", "wrong type!");

        }}
        fields.append(field);
    }
    return fields;
}

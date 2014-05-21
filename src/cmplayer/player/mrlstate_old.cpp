#include "mrlstate_old.hpp"

using namespace MrlStateHelpers;

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

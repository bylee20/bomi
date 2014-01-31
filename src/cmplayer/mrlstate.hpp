#ifndef MRLSTATE_HPP
#define MRLSTATE_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "mrl.hpp"
#include "submisc.hpp"

class MrlStateV1 : public QObject {
	Q_OBJECT
	Q_PROPERTY(QDateTime last_played_date_time MEMBER last_played_date_time)
	Q_PROPERTY(int resume_position MEMBER resume_position)
	Q_PROPERTY(int play_speed MEMBER play_speed NOTIFY playSpeedChanged)

	Q_PROPERTY(InterpolatorType video_interpolator MEMBER video_interpolator NOTIFY videoInterpolatorChanged)
	Q_PROPERTY(InterpolatorType video_chroma_upscaler MEMBER video_chroma_upscaler NOTIFY videoChromaUpscalerChanged)
	Q_PROPERTY(VideoRatio video_aspect_ratio MEMBER video_aspect_ratio NOTIFY videoAspectRatioChanged)
	Q_PROPERTY(VideoRatio video_crop_ratio MEMBER video_crop_ratio NOTIFY videoCropRatioChanged)
	Q_PROPERTY(DeintMode video_deinterlacing MEMBER video_deinterlacing NOTIFY videoDeinterlacingChanged)
	Q_PROPERTY(Dithering video_dithering MEMBER video_dithering NOTIFY videoDitheringChanged)
	Q_PROPERTY(QPoint video_offset MEMBER video_offset NOTIFY videoOffsetChanged)
	Q_PROPERTY(VerticalAlignment video_vertical_alignment MEMBER video_vertical_alignment NOTIFY videoVerticalAlignmentChanged)
	Q_PROPERTY(HorizontalAlignment video_horizontal_alignment MEMBER video_horizontal_alignment NOTIFY videoHorizontalAlignmentChanged)
	Q_PROPERTY(VideoColor video_color MEMBER video_color NOTIFY videoColorChanged)
	Q_PROPERTY(ColorRange video_range MEMBER video_range NOTIFY videoRangeChanged)

	Q_PROPERTY(int audio_volume MEMBER audio_volume NOTIFY audioVolumeChanged)
	Q_PROPERTY(int audio_amplifier MEMBER audio_amplifier NOTIFY audioAmpChanged)
	Q_PROPERTY(int audio_sync MEMBER audio_sync NOTIFY audioSyncChanged)
	Q_PROPERTY(int audio_track MEMBER audio_track)
	Q_PROPERTY(bool audio_muted MEMBER audio_muted NOTIFY audioMutedChanged)
	Q_PROPERTY(bool audio_volume_normalizer MEMBER audio_volume_normalizer NOTIFY audioVolumeNormalizerChanged)
	Q_PROPERTY(bool audio_tempo_scaler MEMBER audio_tempo_scaler NOTIFY audioTempoScalerChanged)
	Q_PROPERTY(ChannelLayout audio_channel_layout MEMBER audio_channel_layout NOTIFY audioChannelLayoutChanged)

	Q_PROPERTY(VerticalAlignment sub_alignment MEMBER sub_alignment NOTIFY subAlignmentChanged)
	Q_PROPERTY(SubtitleDisplay sub_display MEMBER sub_display NOTIFY subDisplayChanged)
	Q_PROPERTY(int sub_position MEMBER sub_position NOTIFY subPositionChanged)
	Q_PROPERTY(int sub_sync MEMBER sub_sync NOTIFY subSyncChanged)
	Q_PROPERTY(SubtitleStateInfo sub_track MEMBER sub_track)
public:
	Mrl mrl;

// play state
	QDateTime last_played_date_time;
	int resume_position = 0;
	int play_speed = 100;

// video state
	VideoRatio video_aspect_ratio = VideoRatio::Source;
	VideoRatio video_crop_ratio = VideoRatio::Source;
	DeintMode video_deinterlacing = DeintMode::Auto;
	InterpolatorType video_interpolator = InterpolatorType::Bilinear;
	InterpolatorType video_chroma_upscaler = InterpolatorType::Bilinear;
	Dithering video_dithering = Dithering::Fruit;
	QPoint video_offset = {0, 0};
	VerticalAlignment video_vertical_alignment = VerticalAlignment::Center;
	HorizontalAlignment video_horizontal_alignment = HorizontalAlignment::Center;
	VideoColor video_color = {0, 0, 0, 0};
	ColorRange video_range = ColorRange::Auto;
	int video_effects = 0;

// audio state
	int audio_amplifier = 100;
	int audio_volume = 100, audio_sync = 0;
	bool audio_muted = false, audio_volume_normalizer = false, audio_tempo_scaler = true;
	ChannelLayout audio_channel_layout = ChannelLayout::Default;
	int audio_track = -1;

// subtitle state
	int sub_position = 100;
	int sub_sync = 0;
	SubtitleDisplay sub_display = SubtitleDisplay::OnLetterbox;
	VerticalAlignment sub_alignment = VerticalAlignment::Bottom;
	SubtitleStateInfo sub_track;

	static const int Version = 1;

	void save() const;

signals:
	void playSpeedChanged();

	void videoInterpolatorChanged();
	void videoAspectRatioChanged();
	void videoCropRatioChanged();
	void videoDeinterlacingChanged();
	void videoChromaUpscalerChanged();
	void videoDitheringChanged();
	void videoColorChanged();
	void videoOffsetChanged();
	void videoVerticalAlignmentChanged();
	void videoHorizontalAlignmentChanged();
	void videoRangeChanged();

	void audioVolumeChanged();
	void audioAmpChanged();
	void audioMutedChanged();
	void audioSyncChanged();
	void audioVolumeNormalizerChanged();
	void audioTempoScalerChanged();
	void audioChannelLayoutChanged();
	void audioTrackChanged();

	void subPositionChanged();
	void subSyncChanged();
	void subDisplayChanged();
	void subAlignmentChanged();
	void subInternalTrackChanged();
	void subExternalTracksChanged();
	void subLoadedTracksChanged();
};

using MrlState = MrlStateV1;

namespace MrlStateHelpers {

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

static inline QString toSql(const QString &text) {
	return _L('\'') % QString(text).replace(_L('\''), _L("''")) % _L('\'');
}


static inline QString toSql(const QDateTime &dt) {
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

static inline QString dateTimeFromSql(qint64 dt) {
#define XT(s) ((dt >> S_##s) & ((1 << B_##s)-1))
	const QString q("%1");
	auto pad = [&q] (int v, int n) -> QString { return q.arg(v, n, 10, _L('0')); };
	return pad(XT(Year), 4) % _L('/') % pad(XT(Month), 2) % _L('/') % pad(XT(Day), 2) % _L(' ')
			% pad(XT(Hour), 2) % _L(':') % pad(XT(Min), 2) % _L(':') % pad(XT(Sec), 2);
#undef XT
}

static inline QString toSql(qint8 integer) { return QString::number(integer); }
static inline QString toSql(qint16 integer) { return QString::number(integer); }
static inline QString toSql(qint32 integer) { return QString::number(integer); }
static inline QString toSql(qint64 integer) { return QString::number(integer); }

static inline QString toSql(const QPoint &p) {
	return _L('\'') % QString::number(p.x()) % "," % QString::number(p.y()) % _L('\'');
}

static inline QPoint pointFromSql(const QString &str) {
	auto index = str.indexOf(','); Q_ASSERT(index != -1);
	return {str.midRef(0, index).toInt(), str.midRef(index+1).toInt()};
}

struct Field {
	QString name() const { return m_name; }
	QString type() const { return m_type; }
	QString toSql(const QVariant &var) const { return m_toSql(var); }
	QVariant fromSql(const QVariant &var) const { return m_fromSql(var, m_variantType); }
	const char *property() const { return m_property; }
private:
	template<typename T> friend QList<Field> fields();
	static QVariant pass(const QVariant &var, int) { return var; }
	QString m_name, m_type;
	const char *m_property = nullptr;
	QString (*m_toSql)(const QVariant&);
	QVariant (*m_fromSql)(const QVariant&, int) = pass;
	int m_variantType = QVariant::Invalid;
};

//static inline QString nameList(const QList<Field> &list) {
//	QString ret;
//	int size = 0;
//	for (int i=0; i<list.size(); ++i)
//		size += list[i].name().size() + 2;
//	ret.reserve(size);
//	for (int i=0; i<list.size(); ++i) {
//		ret += list[i].name();
//		ret += _L(", ");
//	}
//	ret.chop(2);
//	return ret;
//}

////static inline QString findQuery(const Mrl )

//static inline QString listToCreateTable(const QList<Field> &list) {
//	QString ret;
//	int size = 0;
//	for (int i=0; i<list.size(); ++i)
//		size += list[i].name().size() + list[i].type().size() + 3;
//	ret.reserve(size);
//	for (int i=0; i<list.size(); ++i) {
//		ret += list[i].name();
//		ret += _L(' ');
//		ret += list[i].type();
//		ret += _L(", ");
//	}
//	ret.chop(2);
//	return ret;
//}

QList<MrlState*> _ImportMrlStatesFromPreviousVersion(int version, QSqlDatabase db);

template<typename T>
static QList<Field> fields() {
	static QList<Field> fields;
	if (fields.isEmpty()) {
		auto &metaObject = T::staticMetaObject;
		const int count = metaObject.propertyCount();
		const int offset = metaObject.propertyOffset();
		fields.reserve(count - offset);
		for (int i=offset; i<count; ++i) {
			auto p = metaObject.property(i);
			Field field;
			field.m_name = p.name();
			field.m_property = p.name();
			field.m_type = "INTEGER";
			field.m_variantType = p.type();
			switch (p.type()) {
			case QVariant::Int:
				field.m_toSql = [] (const QVariant &var) { return toSql(var.toInt()); };
				break;
			case QVariant::LongLong:
				field.m_toSql = [] (const QVariant &var) { return toSql(var.toLongLong()); };
				break;
			case QVariant::Bool:
				field.m_toSql = [] (const QVariant &var) { return toSql((int)var.toBool()); };
				break;
			case QVariant::Point:
				field.m_toSql = [] (const QVariant &var) { return toSql(var.toPoint()); };
				field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return pointFromSql(var.toString()); };
				break;
			case QVariant::DateTime:
				field.m_toSql = [] (const QVariant &var) { return toSql(var.toDateTime()); };
				field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return dateTimeFromSql(var.toLongLong()); };
				break;
			case QVariant::String:
				field.m_type = "TEXT";
				field.m_toSql = [] (const QVariant &var) { return toSql(var.toString()); };
				break;
			default: {
				Q_ASSERT(p.type() == QVariant::UserType);
				const auto type = p.userType();
				field.m_variantType = p.userType();
				if (_IsEnumTypeId(type)) {
					field.m_toSql = [] (const QVariant &var) { return toSql(var.toInt()); };
					field.m_fromSql = [] (const QVariant &var, int type) -> QVariant { const int i = var.toInt(); return QVariant(type, &i); };
				} else if (type == qMetaTypeId<VideoColor>()) {
					field.m_toSql = [] (const QVariant &var) {
						VideoColor color = var.value<VideoColor>(); color.clamp();
#define PACK(p, s) ((0xffu & ((quint32)color.p() + 100u)) << s)
						return toSql(qint64(PACK(brightness, 24) | PACK(contrast, 16) | PACK(saturation, 8) | PACK(hue, 0)));
#undef PACK
					};
					field.m_fromSql = [] (const QVariant &var, int) -> QVariant {
						const auto v = var.toLongLong();
#define UNPACK(p, s) (((v >> s) & 0xff) - 100)
						return QVariant::fromValue(VideoColor(UNPACK(v, 24), UNPACK(v, 16), UNPACK(v, 8), UNPACK(v, 0)));
#undef UNPACK
					};
				} else if (type == qMetaTypeId<SubtitleStateInfo>()) {
					field.m_type = "TEXT";
					field.m_toSql = [] (const QVariant &var) { return toSql(var.value<SubtitleStateInfo>().toString()); };
					field.m_fromSql = [] (const QVariant &var, int) -> QVariant { return QVariant::fromValue(SubtitleStateInfo::fromString(var.toString())); };
				} else
					Q_ASSERT_X(false, "HistoryDatabaseModel::HistoryDatabaseModel()", "wrong type!");

			}}
			fields.append(field);
		}
	}
	return fields;
}

}

#endif // MRLSTATE_HPP

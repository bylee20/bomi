#include "mrlstate.hpp"
#include "appstate.hpp"
#include "mrlstate_old.hpp"

using namespace MrlStateHelpers;

QList<MrlStateProperty> MrlState::restorableProperties() {
	QList<MrlStateProperty> properties;
	properties.reserve(staticMetaObject.propertyCount());
	auto add = [&properties] (const char *name, const QString &info) {
		const int index = staticMetaObject.indexOfProperty(name);
		Q_ASSERT(index != -1);
		properties.append(MrlStateProperty{staticMetaObject.property(index), info});
	};
#define ADD(n, i) add(#n, i)
	ADD(play_speed, qApp->translate("MrlState", "Playback Speed"));

	ADD(video_interpolator, qApp->translate("MrlState", "Video Interpolator"));
	ADD(video_chroma_upscaler,  qApp->translate("MrlState", "Video Chroma Upscaler"));
	ADD(video_aspect_ratio,  qApp->translate("MrlState", "Video Aspect Ratio"));
	ADD(video_crop_ratio,  qApp->translate("MrlState", "Video Crop Ratio"));
	ADD(video_deinterlacing,  qApp->translate("MrlState", "Video Deinterlacing"));
	ADD(video_dithering,  qApp->translate("MrlState", "Video Dithering"));
	ADD(video_offset,  qApp->translate("MrlState", "Video Screen Position"));
	ADD(video_vertical_alignment,  qApp->translate("MrlState", "Video Vertical Alignment"));
	ADD(video_horizontal_alignment,  qApp->translate("MrlState", "Video Horizontal Alignment"));
	ADD(video_color,  qApp->translate("MrlState", "Video Color Adjustment"));
	ADD(video_range,  qApp->translate("MrlState", "Video Color Range"));

	ADD(audio_volume,  qApp->translate("MrlState", "Audio Volume"));
	ADD(audio_amplifier,  qApp->translate("MrlState", "Audio Amp"));
	ADD(audio_sync,  qApp->translate("MrlState", "Audio Sync"));
	ADD(audio_track,  qApp->translate("MrlState", "Audio Track"));
	ADD(audio_muted,  qApp->translate("MrlState", "Audio Mute"));
	ADD(audio_volume_normalizer,  qApp->translate("MrlState", "Audio Volume Normalizer"));
	ADD(audio_tempo_scaler,  qApp->translate("MrlState", "Audio Tempo Scaler"));
	ADD(audio_channel_layout,  qApp->translate("MrlState", "Audio Channel Layout"));

	ADD(sub_track,  qApp->translate("MrlState", "Subtitle Track"));
	ADD(sub_alignment,  qApp->translate("MrlState", "Subtitle Alignment"));
	ADD(sub_display,  qApp->translate("MrlState", "Subtitle Display"));
	ADD(sub_position,  qApp->translate("MrlState", "Subtitle Position"));
	ADD(sub_sync,  qApp->translate("MrlState", "Subtitle Sync"));

	return properties;
}

template<typename T> static inline bool _Is(int type) { return qMetaTypeId<T>() == type; }

QList<MrlField> MrlField::list() {
	static QList<MrlField> fields;
	if (fields.isEmpty()) {
		MrlState default_;
		auto &metaObject = MrlState::staticMetaObject;
		const int count = metaObject.propertyCount();
		const int offset = metaObject.propertyOffset();
		Q_ASSERT(offset == 1);
		fields.reserve(count - offset);
		for (int i=offset; i<count; ++i) {
			MrlField field;
			field.m_property = metaObject.property(i);
			field.m_type = "INTEGER";
			field.m_default = field.m_property.read(&default_);
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
				field.m_type = "TEXT";
				field.m_toSql = [] (const QVariant &var) { return _ToSql(var.toString()); };
				break;
			default: {
				Q_ASSERT(field.m_property.type() == QVariant::UserType);
				const auto type = field.m_property.userType();
				if (_GetEnumFunctionsForSql(type, field.m_toSql, field.m_fromSql)) {
					field.m_type = "TEXT";
				} else if (_Is<VideoColor>(type)) {
					field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<VideoColor>().packed()); };
					field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
						return var.isNull() ? def : QVariant::fromValue(VideoColor::fromPacked(var.toLongLong()));
					};
				} else if (_Is<SubtitleStateInfo>(type)) {
					field.m_type = "TEXT";
					field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<SubtitleStateInfo>().toString()); };
					field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
						return var.isNull() ? def : QVariant::fromValue(SubtitleStateInfo::fromString(var.toString()));
					};
				} else if (_Is<Mrl>(type)) {
					field.m_type = "TEXT PRIMARY KEY NOT NULL";
					field.m_toSql = [] (const QVariant &var) { return _ToSql(var.value<Mrl>().toString()); };
					field.m_fromSql = [] (const QVariant &var, const QVariant &def) -> QVariant {
						return var.isNull() ? def : QVariant::fromValue(Mrl::fromString(var.toString()));
					};
				} else
					Q_ASSERT_X(false, "HistoryDatabaseModel::HistoryDatabaseModel()", "wrong type!");

			}}
			fields.append(field);
		}
	}
	return fields;
}


namespace MrlStateHelpers {

std::tuple<MrlState*, QList<MrlState*>> _ImportMrlStatesFromPreviousVersion(int version, QSqlDatabase db) {
	std::tuple<MrlState*, QList<MrlState*>> tuple;
	MrlState *&app = std::get<0>(tuple);
	app = new MrlState;
	QList<MrlState*> &states = std::get<1>(tuple);
	if (version < 1) {
		AppStateOld as;
		QSettings set;
		set.beginGroup("history");
		const int size = set.beginReadArray("list");
		states.reserve(size);
		auto fill = [&as] (MrlState *state) {
			state->play_speed = as.playback_speed;

			state->video_aspect_ratio = as.video_aspect_ratio;
			state->video_crop_ratio = as.video_crop_ratio;
			state->video_deinterlacing = as.video_deinterlacing;
			state->video_interpolator = as.video_interpolator;
			state->video_chroma_upscaler = as.video_chroma_upscaler;
			state->video_dithering = as.video_dithering;
			state->video_offset = as.video_offset;
			state->video_vertical_alignment = as.video_vertical_alignment;
			state->video_horizontal_alignment = as.video_horizontal_alignment;
			state->video_color = as.video_color;
			state->video_range = as.video_range;
			state->video_effects = as.video_effects;

			state->audio_amplifier = as.audio_amplifier;
			state->audio_volume = as.audio_volume;
			state->audio_sync = as.audio_sync;
			state->audio_muted = as.audio_muted;
			state->audio_volume_normalizer = as.audio_volume_normalizer;
			state->audio_tempo_scaler = as.audio_tempo_scaler;
			state->audio_channel_layout = as.audio_channel_layout;

			state->sub_position = as.sub_position;
			state->sub_sync = as.sub_sync;
			state->sub_display = as.sub_display;
			state->sub_alignment = as.sub_alignment;
			return state;
		};
		fill(app);
		for (int i=0; i<size; ++i) {
			set.setArrayIndex(i);
			const Mrl mrl = set.value("mrl", QString()).toString();
			if (mrl.isEmpty())
				continue;
			auto state = new MrlState;
			fill(state);
			state->mrl = mrl;
			state->last_played_date_time = set.value("date", QDateTime()).toDateTime();
			state->resume_position = set.value("stopped-position", 0).toInt();
			states.append(state);
		}
	} else if (version < 2) {
		QSqlQuery query(db);
		db.transaction();
		query.exec("SELECT * FROM app LIMIT 1");
		app = new MrlState;
		MrlStateV1 prev;
		const auto fields = MrlFieldV1::list();
		if (query.next()) {
			_FillMrlStateFromQuery<MrlStateV1>(&prev, fields, query);
			prev.fillCurrentVersion(app);
		}
		query.exec("SELECT *, (SELECT COUNT(*) FROM state) as total FROM state");
		if (!query.next())
			return tuple;
		const int rows = query.value("total").toInt();
		query.seek(-1);
		states.reserve(rows);
		while (query.next()) {
			_FillMrlStateFromQuery(&prev, fields, query);
			auto state = new MrlState;
			prev.fillCurrentVersion(state);
			states.append(state);
		}
		db.rollback();
	}
	return tuple;
}

}

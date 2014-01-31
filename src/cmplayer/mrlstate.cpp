#include "mrlstate.hpp"
#include "appstate.hpp"

namespace MrlStateHelpers {

QList<MrlState*> _ImportMrlStatesFromPreviousVersion(int version, QSqlDatabase db) {
	QList<MrlState*> states;
	if (version < MrlStateV1::Version) {
		AppStateOld as;
		QSettings set;
		set.beginGroup("history");
		const int size = set.beginReadArray("list");
		states.reserve(size+1);
		auto make = [&as] () {
			auto state = new MrlState;
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
		states.append(make());
		for (int i=0; i<size; ++i) {
			set.setArrayIndex(i);
			const Mrl mrl = set.value("mrl", QString()).toString();
			if (mrl.isEmpty())
				continue;
			auto state = make();
			state->mrl = mrl;
			state->last_played_date_time = set.value("date", QDateTime()).toDateTime();
			state->resume_position = set.value("stopped-position", 0).toInt();
			states.append(state);
		}
	}
	Q_UNUSED(db);
	return states;
}

}

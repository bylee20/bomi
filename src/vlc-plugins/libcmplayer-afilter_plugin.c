#define __PLUGIN__
#define MODULE_STRING "cmplayer-afilter"

// gcc -I/Applications/VLC.app/Contents/MacOS/include -I. -L/Applications/VLC.app/Contents/MacOS/lib -std=c99 -lvlc -lvlccore -dynamiclib  cmplayer-afilter.c -o libcmplayer-afilter_plugin.dylib

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>
#include <vlc_filter.h>

typedef struct _AudioBuffer {
	void *block;
	void *data;
	int samples;
	int size;
} AudioBuffer;

typedef struct _AudioUtil {
	void *af;
	AudioBuffer *(*allocBuffer)(void *af, int size);
	void (*freeBuffer)(void *af, AudioBuffer *buffer);
	int scaletempoEnabled;
} AudioUtil;

typedef struct _AudioFormat {
	int channels;
	int sampleRate;
} AudioFormat;

typedef struct filter_sys_t {
	AudioUtil *util;
	AudioBuffer *(*process)(void *data, AudioBuffer *in);
	void (*prepare)(void *data, const AudioFormat *format);
	void *opaque;
} Data;

static int ctor (vlc_object_t *obj);
static void dtor (vlc_object_t *obj);
static block_t *process(filter_t *af, block_t *in);
static AudioBuffer *allocBuffer(void *af, int size);
static void freeBuffer(void */*af*/, AudioBuffer *buffer);


vlc_module_begin ()
	set_description(MODULE_STRING)
	set_shortname(MODULE_STRING)
	set_category( CAT_AUDIO )
	set_subcategory( SUBCAT_AUDIO_AFILTER )
	set_capability( "audio filter", 0 )
	add_shortcut( MODULE_STRING )

	add_string(MODULE_STRING "-cb-prepare", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-cb-process", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-util", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-data", "0", NULL, "", "", true)
		change_volatile()
	set_callbacks(ctor, dtor)
vlc_module_end ()

static int ctor(vlc_object_t *obj) {
	filter_t *af = (filter_t*)obj;

	if (af->fmt_in.audio.i_format != VLC_CODEC_FL32
		|| af->fmt_out.audio.i_format != VLC_CODEC_FL32) {
		af->fmt_in.audio.i_format = VLC_CODEC_FL32;
		af->fmt_out.audio.i_format = VLC_CODEC_FL32;
		msg_Warn( af, "bad input or output format" );
		return VLC_EGENERIC;
	}

	if (!AOUT_FMTS_SIMILAR(&af->fmt_in.audio, &af->fmt_out.audio)) {
		memcpy(&af->fmt_out.audio, &af->fmt_in.audio, sizeof(audio_sample_format_t));
		msg_Warn( af, "input and output formats are not similar" );
		return VLC_EGENERIC;
	}

	af->pf_audio_filter = process;

	Data *d = malloc(sizeof(Data));
	af->p_sys = d;
	if(!d)
		return VLC_ENOMEM;

	char *str = 0;
	str = var_CreateGetString(af->p_parent, MODULE_STRING "-cb-process" );
	d->process = (AudioBuffer *(*)(void *, AudioBuffer *))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(af->p_parent, MODULE_STRING "-cb-prepare" );
	d->prepare = (void (*)(void *, const AudioFormat*))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(af->p_parent, MODULE_STRING "-util" );
	d->util = (AudioUtil*)(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(af->p_parent, MODULE_STRING "-data" );
	d->opaque = (void *)(intptr_t)atoll(str);
	free(str);

	if (d->util) {
		d->util->af = af;
		d->util->allocBuffer = allocBuffer;
		d->util->freeBuffer = freeBuffer;
	}

	AudioFormat format;
	format.channels = aout_FormatNbChannels(&af->fmt_in.audio);
	format.sampleRate = af->fmt_in.audio.i_rate;
	d->prepare(d->opaque, &format);

	return VLC_SUCCESS;
}

static void fillBufferWithBlock(filter_t *af, AudioBuffer *buffer, block_t *block) {
	VLC_UNUSED(af);
	buffer->block = block;
	buffer->data = block->p_buffer;
	buffer->size = block->i_buffer;
	buffer->samples = block->i_nb_samples;
}

static block_t *process(filter_t *af, block_t *in) {
	Data *d = af->p_sys;
	AudioBuffer *buffer = malloc(sizeof(AudioBuffer));
	fillBufferWithBlock(af, buffer, in);
	AudioBuffer *ret = d->process(d->opaque, buffer);
	if (ret) {
		block_t *out = ret->block;
		free(ret);
		out->i_buffer = ret->size;
		out->i_nb_samples = ret->samples;
		out->i_dts = in->i_dts;
		out->i_pts = in->i_pts;
		out->i_length = in->i_length;
		return out;
	} else {
		free(buffer);
		return in;
	}
}

static void dtor(vlc_object_t *obj) {
	filter_t *af = (filter_t*)obj;
	free(af->p_sys);
}

static AudioBuffer *allocBuffer(void *af, int size) {
	AudioBuffer *buffer = malloc(sizeof(AudioBuffer));
	block_t *block = filter_NewAudioBuffer((filter_t*)af, size);
	buffer->block = block;
	buffer->data = block->p_buffer;
	buffer->size = block->i_buffer;
	buffer->samples = block->i_nb_samples;
	return buffer;
}

static void freeBuffer(void *af, AudioBuffer *buffer) {
	VLC_UNUSED(af);
	block_Release(buffer->block);
	free(buffer);
}

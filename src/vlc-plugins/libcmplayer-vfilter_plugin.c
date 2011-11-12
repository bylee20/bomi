#define __PLUGIN__
#define MODULE_STRING "cmplayer-vfilter"

// gcc -I/Applications/VLC.app/Contents/MacOS/include -I. -L/Applications/VLC.app/Contents/MacOS/lib -std=c99 -lvlc -lvlccore -dynamiclib  cmplayer-afilter.c -o libcmplayer-afilter_plugin.dylib

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>

typedef struct filter_sys_t {
	void (*process)(void *data, void **planes);
//	void (*prepare)(void *data, const AudioFormat *format);
	void *opaque;
} Data;

static int ctor (vlc_object_t *obj);
static void dtor (vlc_object_t *obj);
static picture_t *process(filter_t *vf, picture_t *in);

vlc_module_begin ()
set_description(MODULE_STRING)
set_shortname(MODULE_STRING)
set_category(CAT_VIDEO)
set_subcategory(SUBCAT_VIDEO_VFILTER)
set_capability( "video filter2", 0 )
add_shortcut(MODULE_STRING)

//add_string(MODULE_STRING "-cb-prepare", "0", NULL, "", "", true)
//change_volatile()
add_string(MODULE_STRING "-cb-process", "0", NULL, "", "", true)
change_volatile()
//add_string(MODULE_STRING "-util", "0", NULL, "", "", true)
//change_volatile()
add_string(MODULE_STRING "-data", "0", NULL, "", "", true)
change_volatile()
set_callbacks(ctor, dtor)
vlc_module_end ()

static int ctor(vlc_object_t *obj) {
	filter_t *vf = (filter_t *)obj;
	switch (vf->fmt_in.video.i_chroma) {
	case VLC_CODEC_I420:
	case VLC_CODEC_YV12:
		break;
	default:
		return VLC_EGENERIC;
	}
	switch (vf->fmt_out.video.i_chroma) {
		case VLC_CODEC_I420:
		case VLC_CODEC_YV12:
			break;
		default:
			return VLC_EGENERIC;
	}
	Data *d = malloc(sizeof(Data));
	vf->p_sys = d;
	char *str = var_CreateGetString(vf->p_parent, MODULE_STRING"-cb-process");
	d->process = (void (*)(void *, void **))(intptr_t)atoll(str);
	free(str);
	str = var_CreateGetString(vf->p_parent, MODULE_STRING"-data");
	d->opaque = (void *)(intptr_t)atoll(str);
	free(str);
    vf->pf_video_filter = process;
    return VLC_SUCCESS;
}


static picture_t *process(filter_t *vf, picture_t *in) {
	Data *d = vf->p_sys;
	void *planes[PICTURE_PLANE_MAX];
	for (int i=0; i<in->i_planes; ++i)
		planes[i] = in->p[i].p_pixels;
	d->process(d->opaque, planes);
	return in;
}

static void dtor(vlc_object_t *obj) {
    filter_t *vf = (filter_t *)obj;
	Data *d = vf->p_sys;
    free(d);
}


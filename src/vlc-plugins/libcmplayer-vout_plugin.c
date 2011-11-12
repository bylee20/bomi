#define __PLUGIN__
#define MODULE_STRING "cmplayer-vout"

// gcc -I/Applications/VLC.app/Contents/MacOS/include -I. -L/Applications/VLC.app/Contents/MacOS/lib -std=c99 -lvlc -lvlccore -dynamiclib  cmplayer-vout.c -o libcmplayer-vout_plugin.dylib

#include <assert.h>
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>

typedef struct _Plane {
	int dataPitch, dataLines;
	int framePitch, frameLines;
} Plane;

typedef struct _VideoFormat {
	uint32_t source_fourcc;
	uint32_t output_fourcc;
	int source_bpp;
	int output_bpp;
	int width;
	int height;
	Plane planes[3];
	int planeCount;
	double sar;
	double fps;
} VideoFormat;

typedef struct _VideoUtil {
	void *vd;
	void (*mousePresseEvent)(void *vd, int button);
	void (*mouseReleaseEvent)(void *vd, int button);
	void (*mouseMoveEvent)(void *vd, int x, int y);
} VideoUtil;

typedef struct vout_display_sys_t {
	picture_pool_t *pool;
	VideoUtil *util;
	void *(*lock)(void *sys, void ***planes);
	void (*unlock)(void *sys, void *id, void *const *plane);
	void (*display)(void *sys, void *id);
	void (*prepare)(void *sys, const VideoFormat *format);
	void (*render)(void *sys, void **planes);
	void *opaque;
} Data;

typedef struct picture_sys_t {
	Data *d;
	void *id;
} PicData;

static int ctor(vlc_object_t *);
static void dtor(vlc_object_t *);
static void handleMousePressed(void *vd, int button);
static void handleMouseReleased(void *vd, int button);
static void handleMouseMoved(void *vd, int x, int y);
static picture_pool_t *pool(vout_display_t *, unsigned);
static void display(vout_display_t *, picture_t *);
static void render(vout_display_t *, picture_t *);
static int control(vout_display_t *, int, va_list);
static void manage (vout_display_t *);
static int lock(picture_t *);
static void unlock(picture_t *);

static int ctor(vlc_object_t *object) {
	vout_display_t *vd = (vout_display_t *)object;
	char *str = 0;
	video_format_t fmt = vd->fmt;

	str = var_CreateGetString(vd, MODULE_STRING"-chroma");
	if (VLC_FOURCC(str[0], str[1], str[2], str[3]) == VLC_FOURCC('A', 'U', 'T', 'O')) {
		printf("auto chroma!\n"); fflush(stdout);
		switch (vd->source.i_chroma) {
		case VLC_CODEC_I420:
		case VLC_CODEC_YV12:
			fmt.i_chroma = vd->source.i_chroma;
			break;
		default:
			fmt.i_chroma = VLC_CODEC_I420;
			break;
		}
	} else {
		const vlc_fourcc_t chroma = vlc_fourcc_GetCodecFromString(VIDEO_ES, str);
		if (!chroma) {
			msg_Err(vd, "cmplayer-vout-chroma should be 4 characters long");
			free(str);
			return VLC_EGENERIC;
		}
		fmt.i_chroma = chroma;
	}
	free(str);

	switch (fmt.i_chroma) {
	case VLC_CODEC_RGB15:
		fmt.i_rmask = 0x001f;
		fmt.i_gmask = 0x03e0;
		fmt.i_bmask = 0x7c00;
		break;
	case VLC_CODEC_RGB16:
		fmt.i_rmask = 0x001f;
		fmt.i_gmask = 0x07e0;
		fmt.i_bmask = 0xf800;
		break;
	case VLC_CODEC_RGB24:
		fmt.i_rmask = 0xff0000;
		fmt.i_gmask = 0x00ff00;
		fmt.i_bmask = 0x0000ff;
		break;
	case VLC_CODEC_RGB32:
		fmt.i_rmask = 0xff0000;
		fmt.i_gmask = 0x00ff00;
		fmt.i_bmask = 0x0000ff;
		break;
	default:
		fmt.i_rmask = 0;
		fmt.i_gmask = 0;
		fmt.i_bmask = 0;
		break;
	}

	Data *d = malloc(sizeof(Data));
	vd->sys = d;
	if (!d)
		return VLC_ENOMEM;

	str = var_CreateGetString(vd, MODULE_STRING "-cb-lock" );
	d->lock = (void *(*)(void *, void ***))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(vd, MODULE_STRING "-cb-unlock" );
	d->unlock = (void (*)(void *, void *, void *const *))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(vd, MODULE_STRING "-cb-display" );
	d->display = (void (*)(void *, void *))(intptr_t)atoll(str);
	free(str);
	str = var_CreateGetString(vd, MODULE_STRING "-cb-render" );
	d->render = (void (*)(void *, void **))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(vd, MODULE_STRING "-cb-prepare" );
	d->prepare = (void (*)(void *, const VideoFormat*))(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(vd, MODULE_STRING "-data" );
	d->opaque = (void *)(intptr_t)atoll(str);
	free(str);

	str = var_CreateGetString(vd, MODULE_STRING "-util" );
	d->util = (VideoUtil*)(intptr_t)atoll(str);
	free(str);

	if (d->util) {
		d->util->vd = vd;
		d->util->mousePresseEvent = handleMousePressed;
		d->util->mouseReleaseEvent = handleMouseReleased;
		d->util->mouseMoveEvent = handleMouseMoved;
	}

	picture_t *picture = picture_NewFromFormat(&fmt);
	if (!picture) {
		free(d);
		return VLC_EGENERIC;
	}

	picture->p_sys = malloc(sizeof(picture_sys_t));
	picture->p_sys->id = 0;
	picture->p_sys->d = d;

	picture_pool_configuration_t conf;
	memset(&conf, 0, sizeof(pool));
	conf.picture_count = 1;
	conf.picture = &picture;
	conf.lock = lock;
	conf.unlock = unlock;
	d->pool = picture_pool_NewExtended(&conf);
	if (!d->pool) {
		picture_Release(picture);
		free(d);
		return VLC_EGENERIC;
	}

	vout_display_info_t info = vd->info;
	info.has_hide_mouse = true;

	vd->fmt = fmt;
	vd->info = info;
	vd->pool = pool;
	vd->prepare = render;
	vd->display = display;
	vd->control = control;
	vd->manage  = manage;

	vout_display_SendEventFullscreen(vd, false);
	vout_display_SendEventDisplaySize(vd, fmt.i_width, fmt.i_height, false);

	char fourcc[5] = {0};
	const double fps = (double)fmt.i_frame_rate/(double)fmt.i_frame_rate_base;
	const double sar = (double)vd->source.i_sar_num/(double)vd->source.i_sar_den;

	VideoFormat format;
	format.source_fourcc = vd->source.i_chroma;
	format.output_fourcc = fmt.i_chroma;
	format.source_bpp = vd->source.i_bits_per_pixel;
	format.output_bpp = fmt.i_bits_per_pixel;
	format.width = fmt.i_width;
	format.height = fmt.i_height;
	format.planeCount = picture->i_planes;
	format.sar = sar;
	format.fps = fps;
	for (int i=0; i<picture->i_planes; ++i) {
		const plane_t *const p = &picture->p[i];
		format.planes[i].dataPitch = p->i_pitch;
		format.planes[i].dataLines = p->i_lines;
		format.planes[i].framePitch = p->i_visible_pitch;
		format.planes[i].frameLines = p->i_visible_lines;
	}

	d->prepare(d->opaque, &format);
	vlc_fourcc_to_char(vd->source.i_chroma, fourcc);
	printf("source: %s\n", fourcc); fflush(stdout);
	return VLC_SUCCESS;
}

static void dtor(vlc_object_t *object) {
	vout_display_t *vd = (vout_display_t *)object;
	Data *d = vd->sys;
	if (d->util)
		d->util->vd = 0;
	picture_pool_Delete(d->pool);
	free(d);
}

static picture_pool_t *pool(vout_display_t *vd, unsigned count) {
	VLC_UNUSED(count);
	return vd->sys->pool;
}

static void display(vout_display_t *vd, picture_t *picture)
{
	Data *d = vd->sys;
	if (d->display != NULL)
		d->display(d->opaque, picture->p_sys->id);
	picture_Release(picture);
}

static int control(vout_display_t *vd, int query, va_list args) {
	switch (query) {
	case VOUT_DISPLAY_CHANGE_FULLSCREEN:
	case VOUT_DISPLAY_CHANGE_DISPLAY_SIZE: {
		const vout_display_cfg_t *cfg = va_arg(args, const vout_display_cfg_t *);
		if (cfg->display.width  != vd->fmt.i_width ||
			cfg->display.height != vd->fmt.i_height)
			return VLC_EGENERIC;
		if (cfg->is_fullscreen)
			return VLC_EGENERIC;
		return VLC_SUCCESS;
	}
	default:
		return VLC_EGENERIC;
	}
}

static void render(vout_display_t *vd, picture_t *picture) {
	PicData *pd = picture->p_sys;
	Data *d = pd->d;
	void *planes[PICTURE_PLANE_MAX];
	for (int i=0; i<picture->i_planes; ++i)
		planes[i] = picture->p[i].p_pixels;
	d->render(d->opaque, planes);
}

static void manage(vout_display_t *vd) {
	VLC_UNUSED(vd);
}

static int lock(picture_t *picture) {
	PicData *pd = picture->p_sys;
	Data *d = pd->d;

	void **planes[PICTURE_PLANE_MAX];
	for (int i=0; i<picture->i_planes; ++i)
		planes[i] = (void**)&picture->p[i].p_pixels;
	pd->id = d->lock(d->opaque, planes);
	return VLC_SUCCESS;
}

static void unlock(picture_t *picture) {
	PicData *pd = picture->p_sys;
	Data *d = pd->d;
	void *planes[PICTURE_PLANE_MAX];
	for (int i=0; i<picture->i_planes; ++i)
		planes[i] = picture->p[i].p_pixels;
	if (d->unlock)
		d->unlock(d->opaque, pd->id, planes);
}

static void handleMousePressed(void *vd, int button) {
	vout_display_SendEventMousePressed((vout_display_t*)vd, button);
}

static void handleMouseReleased(void *vd, int button) {
	vout_display_SendEventMouseReleased((vout_display_t*)vd, button);
}

static void handleMouseMoved(void *vd, int x, int y) {
	vout_display_SendEventMouseMoved((vout_display_t*)vd, x, y);
}

vlc_module_begin()
	set_description(MODULE_STRING)
	set_shortname(MODULE_STRING)

	set_category(CAT_VIDEO)
	set_subcategory(SUBCAT_VIDEO_VOUT)
	set_capability("vout display", 0)

	add_string(MODULE_STRING"-chroma", "I420", NULL, "", "", true)
		change_private()
	add_string(MODULE_STRING "-cb-prepare", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-cb-lock", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-cb-unlock", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-cb-display", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-cb-render", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-data", "0", NULL, "", "", true)
		change_volatile()
	add_string(MODULE_STRING "-util", "0", NULL, "", "", true)
		change_volatile()
	set_callbacks(ctor, dtor)
vlc_module_end()


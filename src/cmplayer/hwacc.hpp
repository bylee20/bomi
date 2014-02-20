#ifndef HWACC_HPP
#define HWACC_HPP

#include "stdafx.hpp"
#include <array>
extern "C" {
#include <libavcodec/avcodec.h>
#include <video/img_format.h>
}
#include "enums.hpp"

struct lavc_ctx;		struct AVFrame;
struct mp_image;		struct vd_lavc_hwdec;
struct mp_hwdec_info;	class VideoOutput;
class OpenGLTexture2D;	class VideoFormat;
class VideoFrame;		class VideoFormatData;

class HwAccMixer {
public:
	virtual ~HwAccMixer() {}
	virtual bool upload(const VideoFrame &frame, bool deint) = 0;
	virtual bool directRendering() const = 0;
};

class HwAcc {
public:
	enum Type {VaApiGLX, VdpauX11, Vda};
	virtual ~HwAcc();
	static void initialize();
	static void finalize();
	static QList<AVCodecID> fullCodecList();
	static QList<DeintMethod> fullDeintList();
	static bool supports(AVCodecID codec);
	static bool supports(DeintMethod method);
	static Type backend();
	static QString backendName();
	static const char *codecName(int id);
	static AVCodecID codecId(const char *name);
	static HwAccMixer *createMixer(const QList<OpenGLTexture2D> &textures, const VideoFormat &format);
	static bool adjust(VideoFormatData *data, const mp_image *mpi);
	virtual mp_image *getImage(mp_image *mpi) = 0;
	virtual Type type() const = 0;
	int imgfmt() const;
protected:
	HwAcc(AVCodecID codec);
	virtual bool isOk() const = 0;
	virtual mp_image *getSurface() = 0;
	virtual void *context() const = 0;
	virtual bool fillContext(AVCodecContext *avctx) = 0;
	AVCodecID codec() const {return m_codec;}
	const QSize &size() const {return m_size;}
private:
	static VideoOutput *vo(lavc_ctx *ctx);
	static int probe(vd_lavc_hwdec *hwdec, mp_hwdec_info *info, const char *decoder);
	static int init(lavc_ctx *ctx);
	static void uninit(lavc_ctx *ctx);
	static mp_image *allocateImage(struct lavc_ctx *ctx, int imgfmt, int width, int height);
	friend class PlayEngine;
	friend vd_lavc_hwdec create_vaapi_functions();
	friend vd_lavc_hwdec create_vdpau_functions();
	friend vd_lavc_hwdec create_vda_functions();
	struct Data;
	Data *d;
	AVCodecID m_codec = AV_CODEC_ID_NONE;
	QSize m_size = {0,0};
};

#endif // HWACC_HPP

#ifndef HWACC_VDA_HPP
#define HWACC_VDA_HPP

#include "hwacc.hpp"
#include "videoformat.hpp"

class HwAccVda : public HwAcc {
public:
	HwAccVda(AVCodecID codec);
	~HwAccVda();
	virtual mp_image *getImage(mp_image *mpi) override;
	virtual Type type() const override {return Vda;}
	virtual bool isOk() const override;
	virtual mp_image *getSurface() override;
	virtual bool isAvailable(AVCodecID codec) const { return codec == AV_CODEC_ID_H264; }
	virtual void *context() const override;
	bool fillContext(AVCodecContext *avctx) override;
	void freeContext();
private:
	struct Data;
	Data *d;
};

class VdaMixer : public HwAccMixer {
public:
	VdaMixer(const QList<OpenGLTexture2D> &textures, const VideoFormat &format);
	bool upload(const VideoFrame &frame, bool deint) override;
	static void fill(VideoFormat::Data *data, const mp_image *mpi);
private:
	const QList<OpenGLTexture2D> &m_textures;
};

#endif // HWACC_VDA_HPP

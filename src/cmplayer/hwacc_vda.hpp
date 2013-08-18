#ifndef HWACC_VDA_HPP
#define HWACC_VDA_HPP

#include "hwacc.hpp"

class HwAccVda : public HwAcc {
public:
	HwAccVda(AVCodecID codec);
	~HwAccVda();
	virtual mp_image *getImage(mp_image *mpi) override;
	virtual Type type() const override {return Vda;}
	virtual bool isOk() const override;
	virtual bool check(AVCodecContext *avctx) override;
	virtual mp_image *getSurface() override;
	virtual bool isAvailable(AVCodecID codec) const { return codec == AV_CODEC_ID_H264; }
	virtual void *context() const override;
	bool fillContext(AVCodecContext *avctx);
	void freeContext();
private:
	struct Data;
	Data *d;
};

#endif // HWACC_VDA_HPP

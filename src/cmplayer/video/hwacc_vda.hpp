#ifndef HWACC_VDA_HPP
#define HWACC_VDA_HPP

#include "hwacc.hpp"

class HwAccVda : public HwAcc {
public:
    HwAccVda(AVCodecID codec);
    ~HwAccVda();
    virtual mp_image *getImage(mp_image *mpi) override;
    virtual auto type() const -> Type override {return Vda;}
    virtual auto isOk() const -> bool override;
    virtual mp_image *getSurface() override;
    virtual auto isAvailable(AVCodecID codec) const -> bool { return codec == AV_CODEC_ID_H264; }
    virtual void *context() const override;
    auto fillContext(AVCodecContext *avctx, int w, int h) -> bool override;
    auto freeContext() -> void;
private:
    struct Data;
    Data *d;
};

class VdaMixer : public HwAccMixer {
public:
    VdaMixer(const QSize &size);
    auto upload(const mp_image *mpi, bool deint) -> bool override;
    auto directRendering() const -> bool override { return m_direct; }
private:
    const QList<OpenGLTexture2D> &m_textures;
    bool m_direct = false;
};

#endif // HWACC_VDA_HPP

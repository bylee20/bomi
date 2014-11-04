#ifndef VIDEOFRAMEBUFFEROBJECT_HPP
#define VIDEOFRAMEBUFFEROBJECT_HPP

#include "videoimagepool.hpp"
#include "opengl/openglframebufferobject.hpp"
#include "mpimage.hpp"

class HwAccMixer;

class VideoTexture : public OpenGLTexture2D {
public:
    VideoTexture(OGL::Target target)
        : OpenGLTexture2D(target)
    {
        Q_ASSERT(QOpenGLContext::currentContext());
        create();
    }
    ~VideoTexture();
    auto displaySize() const -> QSize { return m_displaySize; }
    auto directRender(const MpImage &mpi, bool deint) const -> bool;
    auto upload(const MpImage &mpi, bool deint) -> void;
    auto initialize(mp_imgfmt imgfmt, const QSize &size,
                    const OpenGLTextureTransferInfo &info,
                    bool dma, int plane = 0) -> void;
    auto plane() const -> int { return m_plane; }
    auto imgfmt() const -> mp_imgfmt { return m_imgfmt; }
private:
    HwAccMixer *m_mixer = nullptr;
    QSize m_displaySize;
    bool m_dma = false;
    int m_plane = 0;
    mp_imgfmt m_imgfmt = IMGFMT_NONE;
    friend class VideoTexturePool;
};

using VideoTextureCache = VideoImageCache<VideoTexture>;

#endif // VIDEOFRAMEBUFFEROBJECT_HPP

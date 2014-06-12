#include "videotexture.hpp"
#include "hwacc.hpp"

VideoTexture::~VideoTexture()
{
    Q_ASSERT(QOpenGLContext::currentContext());
    delete m_mixer;
    destroy();
}

auto VideoTexture::upload(const mp_image *mpi, bool deint) -> void
{
    static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;
    if (m_mixer) {
        m_mixer->upload(*this, mpi, deint);
    } else if (!(mpi->fields & MP_IMGFIELD_ADDITIONAL)) {
        if (m_dma) {
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
            OpenGLTexture2D::initialize(mpi->planes[m_plane]);
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        } else
            OpenGLTexture2D::upload(mpi->planes[m_plane]);
    }
}

auto VideoTexture::initialize(mp_imgfmt imgfmt, const QSize &size,
                              const OpenGLTextureTransferInfo &info,
                              bool dma, int plane) -> void
{
    delete m_mixer;
    if (!isEmpty()) {
        glBindTexture(target(), 0);
        destroy();
        create();
        glBindTexture(target(), id());
    }
    m_plane = plane;
    m_dma = dma;
    m_imgfmt = imgfmt;
    setAttributes(size.width(), size.height(), info);
    if (imgfmt != IMGFMT_VDA) {
        if (m_dma)
            glTexParameteri(target(), GL_TEXTURE_STORAGE_HINT_APPLE,
                            GL_STORAGE_SHARED_APPLE);
        else
            OpenGLTexture2D::initialize();
    }
    m_mixer = HwAcc::createMixer(m_imgfmt, size);
}

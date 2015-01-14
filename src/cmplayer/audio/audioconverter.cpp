#include "audioconverter.hpp"

#include "misc/tmp.hpp"
#include "tmp/type_test.hpp"
#include "tmp/arithmetic_type.hpp"
#include "enum/clippingmethod.hpp"
extern "C" {
#include <audio/format.h>
#include <audio/audio.h>
}

template<class T>
static auto convert(uchar *dst, float src) -> void
{
    auto value = (T*)dst;
    if (tmp::is_floating_point<T>())
        *value = src;
    else
        *value = src * _Max<T>() + 0.5;
}


auto AudioConverter::setFormat(const AudioBufferFormat &format) -> void
{
    if (!_Change(m_format, format))
        return;
    m_dst.setForRawData(format.type, format.channels.num);
    m_convert = [=] () -> Convert {
        switch (format.type) {
        case AF_FORMAT_S8:
            return convert<char>;
        case AF_FORMAT_S16:
        case AF_FORMAT_S16P:
            return convert<qint16>;
        case AF_FORMAT_S32:
        case AF_FORMAT_S32P:
            return convert<qint32>;
        case AF_FORMAT_FLOAT:
        case AF_FORMAT_FLOATP:
            return convert<float>;
        case AF_FORMAT_DOUBLE:
        case AF_FORMAT_DOUBLEP:
            return convert<double>;
        default:
            return nullptr;
        }
    }();
    Q_ASSERT(m_convert != nullptr);
}

auto AudioConverter::run(AudioBuffer *in) -> AudioBuffer*
{
    if (m_format.type == AF_FORMAT_FLOAT)
        return in;
    const auto min_bytes = in->samples() * m_dst.bps();
    if (min_bytes > (int)m_data.size())
        m_data.resize(min_bytes * 1.2 + 1);
    m_dst.setRawFrames(in->frames());
    if (m_dst.planes() > 1) {
        for (int ch = 0; ch < m_dst.channels(); ++ch) {
            float *src = in->p() + ch;
            uchar *dst = m_data.data() + ch * in->frames();
            m_dst.setRawData(dst, ch);
            for (int i = 0; i < in->frames(); ++i) {
                m_convert(dst, *src);
                src += m_dst.channels();
                dst += m_dst.bps();
            }
        }
    } else {
        uchar *dst = m_data.data();
        m_dst.setRawData(dst);
        for (auto it = in->begin(); it != in->end(); ++it) {
            m_convert(dst, *it);
            dst += m_dst.bps();
        }
    }
    return &m_dst;
}

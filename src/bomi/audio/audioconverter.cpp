#include "audioconverter.hpp"

#include "misc/tmp.hpp"
#include "tmp/type_traits.hpp"
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
    m_convert = [=] () -> Convert {
        switch (format.type()) {
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

auto AudioConverter::passthrough(const AudioBufferPtr &/*in*/) const -> bool
{
    return m_format.type() == AF_FORMAT_FLOAT;
}

auto AudioConverter::run(AudioBufferPtr &in) -> AudioBufferPtr
{
    if (m_format.type() == AF_FORMAT_FLOAT)
        return in;
    auto dest = newBuffer(m_format, in->frames());
    auto sview = in->constView<float>();
    if (dest->isPlanar()) {
        for (int ch = 0; ch < dest->channels(); ++ch) {
            const float *src = sview.plane() + ch;
            uchar *dst = dest->data()[ch];
            for (int i = 0; i < in->frames(); ++i) {
                m_convert(dst, *src);
                src += dest->channels();
                dst += dest->bps();
            }
        }
    } else {
        uchar *dst = dest->data()[0];
        for (auto it = sview.begin(); it != sview.end(); ++it) {
            m_convert(dst, *it);
            dst += dest->bps();
        }
    }
    return dest;
}

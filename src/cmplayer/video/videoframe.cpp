#include "videoframe.hpp"
#include <stdlib.h>
extern "C" {
#include <libswscale/swscale.h>
#include <video/fmt-conversion.h>
#include <video/img_format.h>
#include <video/mp_image.h>
#include <video/img_fourcc.h>
}

VideoFrame::VideoFrame::Data::Data(bool take, mp_image *mpi, const VideoFormat &format, double pts, int field)
: format(format), pts(pts), field(field) {
    data[0] = mpi->planes[0];
    data[1] = mpi->planes[1];
    data[2] = mpi->planes[2];
    data[3] = mpi->planes[3];
    this->mpi = take ? mpi : mp_image_new_ref(mpi);
}

static QImage convertImage(const QImage &image) {
    switch (image.format()) {
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB32:
        return image;
    default:
        return image.convertToFormat(QImage::Format_ARGB32);
    }
}

VideoFrame::VideoFrame::Data::Data(const QImage &image)
: mpi(nullptr), image(convertImage(image)), format(this->image) {
    data[0] = this->image.bits();
}

VideoFrame::VideoFrame::Data::Data(const Data &other)
: QSharedData(other), image(other.image), format(other.format) {
    field = other.field;
    pts = other.pts;
    data[0] = other.data[0];
    data[1] = other.data[1];
    data[2] = other.data[2];
    data[3] = other.data[3];
    if (other.mpi)
        mpi = mp_image_new_ref(other.mpi);
    if (!other.buffer.isEmpty()) {
        int offset = 0;
        buffer.resize(other.buffer.size());
        for (int i=0; i<format.planes(); ++i) {
            data[i] = (uchar*)buffer.data() + offset;
            offset += format.bytesPerPlain(i);
        }
        memcpy(buffer.data(), other.buffer.data(), buffer.size());
    }
}

VideoFrame::Data::~Data() {
    if (mpi)
        mp_image_unrefp(&mpi);
}

QImage VideoFrame::toImage() const {
    if (!d->image.isNull())
        return d->image;
    if (!d->mpi || d->format.isEmpty())
        return QImage();
    SwsContext *sws = sws_getCachedContext(nullptr
        , d->format.width(), d->format.height(), imgfmt2pixfmt(d->format.type())
        , d->format.width(), d->format.height(), AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    QImage image(d->format.size(), QImage::Format_RGB888);
    const uchar *srcData[] = {data(0), data(1), data(2)};
    const int srcStride[] = {d->format.bytesPerLine(0), d->format.bytesPerLine(1), d->format.bytesPerLine(2)};
    uchar *destData[] = {image.bits()}; const int destStride[] = {image.bytesPerLine()};
    sws_scale(sws, srcData, srcStride, 0, d->format.height(), destData, destStride);
    sws_freeContext(sws);
    return image;
}

void VideoFrame::allocate(const VideoFormat &format) {
    if (format.isEmpty() && d->buffer.isEmpty())
        return;
    if (!d->buffer.isEmpty() && d->format == format)
        return;
    d.detach();
    d->format = format;
    int len = 0;
    int offsets[4] = {0};
    for (int i=0; i<format.planes(); ++i) {
        offsets[i] = len;
        len += format.bytesPerPlain(i);
    }
    d->buffer.resize(len);
    for (int i=0; i< format.planes(); ++i)
        d->data[i] = (uchar*)d->buffer.data() + offsets[i];
}

void VideoFrame::doDeepCopy(const VideoFrame &frame) {
    d.detach();
    Q_ASSERT(d->format == frame.format());
    auto p = d->buffer.data();
    for (int i=0; i<d->format.planes(); ++i) {
        const int len = d->format.bytesPerPlain(i);
        memcpy(p, frame.data(i),  len);
        p += len;
    }
}

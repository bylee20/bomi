#include "charsetdetector.hpp"
#include "misc/log.hpp"
#define HAVE_DLL_EXPORT
#include <chardet.h>

DECLARE_LOG_CONTEXT(Charset)

struct CharsetDetector::Data {
    DetectObj *obj;
    bool detected;
};

CharsetDetector::CharsetDetector(const QByteArray &data)
: d(new Data) {
    d->obj = detect_obj_init();
    d->detected = (::detect(data.data(), &d->obj) == CHARDET_SUCCESS);
}

CharsetDetector::~CharsetDetector() {
    detect_obj_free(&d->obj);
    delete d;
}

auto CharsetDetector::isDetected() const -> bool
{
    return d->detected;
}

auto CharsetDetector::encoding() const -> QString
{

    if (d->detected) {
        const QString enc(_L(d->obj->encoding));
        auto same = [&] (const QLatin1String &e) -> bool
            { return !enc.compare(e, Qt::CaseInsensitive); };
#define FB(from, to) { if (same(from ""_a)) return u"" to ""_q; }
        FB("euc-kr", "windows-949");
        FB("iso-8859-2", "windows-1250");
#undef FB
        return enc;
    }
    return QString();
}

auto CharsetDetector::confidence() const -> double
{
    return d->detected ? d->obj->confidence : 0.0;
}


auto CharsetDetector::detect(const QByteArray &data, double confidence) -> EncodingInfo
{
    CharsetDetector chardet(data);
    if (!chardet.isDetected()) {
        _Info("Failed to detect encoding.");
        return EncodingInfo();
    }
    const auto enc = chardet.encoding();
    const auto conf = chardet.confidence();
    _Info("Encoding detected: %% (confidence: %%)", enc, conf);
    if (conf >= confidence)
        return EncodingInfo::fromName(enc);
    _Info("Through away detected encoding for low confidence < %%.", confidence);
    return EncodingInfo();
}

auto CharsetDetector::detect(const QString &fileName, double confidence, int size) -> EncodingInfo
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        _Error("Cannot open file: %%", fileName);
        return EncodingInfo();
    }
    _Info("Trying encoding autodetection: %%", fileName);
    size = -1;
    if (size < 0)
        size = file.size();
    return detect(file.read(size), confidence);
}

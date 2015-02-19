#ifndef CHARSETDETECTOR_HPP
#define CHARSETDETECTOR_HPP

#include "encodinginfo.hpp"

class CharsetDetector {
public:
    CharsetDetector(const QByteArray &data);
    ~CharsetDetector();
    auto isDetected() const -> bool;
    auto encoding() const -> QString;
    auto confidence() const -> double;
    static auto detect(const QString &fileName, double confidence = 0.6,
                       int size = 1024*500) -> EncodingInfo;
    static auto detect(const QByteArray &data, double confidence = 0.6) -> EncodingInfo;
private:
    struct Data;
    Data *d;
};

#endif // CHARSETDETECTOR_HPP

#ifndef VAAPIPOSTPROCESSOR_HPP
#define VAAPIPOSTPROCESSOR_HPP

#include "videofilter.hpp"
#include "hwacc_vaapi.hpp"

#ifdef USE_VAVPP

class DeintOption;

class VaApiPostProcessor final : public VideoFilter, public VaApiStatusChecker {
public:
    VaApiPostProcessor();
    ~VaApiPostProcessor();
    auto setDeintOption(const DeintOption &option) -> void;
    auto setAvaiableList(const QList<VFType> &filters) -> void;
    auto process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) -> ([\w\d]+) override;
private:
    mp_image *render(const VideoFrame &in);
    struct Data;
    Data *d;
};

#endif

#endif // VAAPIPOSTPROCESSOR_HPP

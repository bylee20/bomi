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
    void setDeintOption(const DeintOption &option);
    void setAvaiableList(const QList<VFType> &filters);
    bool process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) override;
private:
    mp_image *render(const VideoFrame &in);
    struct Data;
    Data *d;
};

#endif

#endif // VAAPIPOSTPROCESSOR_HPP

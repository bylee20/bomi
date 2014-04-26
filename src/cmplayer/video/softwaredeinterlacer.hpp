#ifndef SOFTWAREDEINTERLACER_HPP
#define SOFTWAREDEINTERLACER_HPP

#include "stdafx.hpp"

#include "videofilter.hpp"

class DeintOption;        struct mp_image;

class SoftwareDeinterlacer final : public VideoFilter {
    enum Type {Graph, PP, Mark, Pass};
public:
    SoftwareDeinterlacer();
    virtual ~SoftwareDeinterlacer();
    SoftwareDeinterlacer(const SoftwareDeinterlacer &other) = delete;
    SoftwareDeinterlacer &operator = (const SoftwareDeinterlacer &rhs) = delete;
    void setOption(const DeintOption &deint);
protected:
    bool process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) override;
private:
    static double ptsStep(double pts, double prev, int split = 2) {
        if (pts == MP_NOPTS_VALUE || prev == MP_NOPTS_VALUE)
            return 0.0;
        const double step = (pts - prev)/double(split);
        return (0.0 < step && step < 0.5) ? step : 0.0;
    }
    int tryIt(Type type);
    void split(const VideoFrame &in, QLinkedList<VideoFrame> &outs);
    bool tryGraph(const VideoFrame &in, QVector<mp_image*> &outs);
    bool tryPostProc(const VideoFrame &in, QVector<mp_image*> &outs);
    struct Data;
    Data *d;
};

#endif // SOFTWAREDEINTERLACER_HPP

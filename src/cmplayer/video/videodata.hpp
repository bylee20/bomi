#ifndef VIDEODATA_HPP
#define VIDEODATA_HPP

#include "mpimage.hpp"
#include "mposdbitmap.hpp"
#include "videoimagepool.hpp"
#include "videoformat.hpp"

using VideoOsdCache = VideoImageCache<MpOsdBitmap>;

class VideoData {
public:
    VideoData() { }
    VideoData(const VideoData &other)
        : m_mpi(other.m_mpi), m_osd(other.m_osd) { }
    VideoData(VideoData &&other)
        : m_mpi(std::move(other.m_mpi)), m_osd(std::move(other.m_osd)) { }
    VideoData &operator = (const VideoData &rhs)
    {
        if (this != &rhs) {
            m_mpi = rhs.m_mpi;
            m_osd = rhs.m_osd;
        }
        return *this;
    }
    VideoData &operator = (VideoData &&rhs)
    {
        m_mpi = std::move(rhs.m_mpi);
        m_osd = std::move(rhs.m_osd);
        return *this;
    }
    auto setOsd(const VideoOsdCache &osd) -> void { m_osd = osd; }
    auto setOsd(VideoOsdCache &&osd) -> void { m_osd = std::move(osd); }
    auto setImage(const MpImage &mpi) -> void { m_mpi = mpi; }
    auto setImage(MpImage &&mpi) -> void { m_mpi = std::move(mpi); }
    auto release() -> void { m_mpi = {}; m_osd = {}; }
    auto hasImage() const -> bool { return !m_mpi.isNull(); }
    auto hasOsd() const -> bool { return !m_osd.isNull(); }
    auto image() const -> const MpImage& { return m_mpi; }
    auto osd() const -> const VideoOsdCache& { return m_osd; }
    auto swap(VideoData &rhs) -> void
        { m_mpi.swap(rhs.m_mpi); m_osd.swap(rhs.m_osd); }
private:
    MpImage m_mpi;
    VideoOsdCache m_osd;
};

#endif // VIDEODATA_HPP

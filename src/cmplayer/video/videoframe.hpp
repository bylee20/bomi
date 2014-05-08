//#ifndef VIDEOFRAME_HPP
//#define VIDEOFRAME_HPP

//#include "stdafx.hpp"
//#include "videoformat.hpp"

//#ifdef None
//#undef None
//#endif

//class VideoFrame {
//public:
//    enum Field {
//        None       = 0,
//        Picture    = 0,
//        Top        = 1,
//        Bottom     = 2,
//        Interlaced = Top | Bottom,
//        Additional = 4,
//        Flipped    = 8
//    };
//    VideoFrame(): d(new Data) {}
//    VideoFrame(mp_image *mpi, int field): d(new Data(mpi, field)) { }
//    explicit VideoFrame(const QImage &image): d(new Data(image)) {}
//    VideoFrame(VideoFrame &&other): d(nullptr) { d.swap(other.d); }
//    VideoFrame(const VideoFrame &other): d(other.d) {}
//    VideoFrame &operator = (const VideoFrame &other) { d = other.d; return *this; }
//    VideoFrame &operator = (VideoFrame &&other) { d.swap(other.d); return *this; }
//    auto toImage() const -> QImage;
//    auto isFlipped() const -> bool { return d->field & Flipped; }
//    auto hasImage() const -> bool {return !d->image.isNull();}
//    auto image() const -> const QImage& {return d->image;}
//    auto data(int i) const -> const uchar* {return d->data[i];}
//    auto format() const -> const VideoFormat& {return d->format;}
//    auto width() const -> int {return d->format.width();}
//    auto height() const -> int {return d->format.height();}
//    auto pts() const -> double {return d->pts;}
//    auto field() const -> int { return d->field; }
//    auto mpi() const -> mp_image* { return d->mpi; }
//    auto isAdditional() const -> bool { return d->field & Additional; }
//    auto isInterlaced() const -> bool { return d->field & Interlaced; }
//    auto isTopField() const -> bool { return d->field & Top; }
//    auto isBottomField() const -> bool { return d->field & Bottom; }
//    auto swap(VideoFrame &other) -> void {d.swap(other.d);}
//    auto allocate(const VideoFormat &format) -> void;
//    auto doDeepCopy(const VideoFrame &frame) -> void;
//    auto isNull() const -> bool { return !d; }
//    auto copyInterlaced(const VideoFrame &frame) -> void { d->field = (d->field & ~Interlaced) | (frame.field() & Interlaced); }
//private:
//    struct Data : public QSharedData {
//        Data() {}
//        Data(mp_image *mpi, int field);
//        Data(const QImage &image);
//        Data(const Data &other);
//        ~Data();
//        Data &operator = (const Data &rhs) = delete;
//        mp_image *mpi = nullptr;
//        QImage image;
//        uchar *data[4] = {0, 0, 0, 0};
//        VideoFormat format;
//        double pts = MP_NOPTS_VALUE;
//        int field = Picture;
//        QByteArray buffer;
//    };
//    QExplicitlySharedDataPointer<Data> d;
//};

//Q_DECLARE_TYPEINFO(VideoFrame, Q_MOVABLE_TYPE);

//#endif // VIDEOFRAME_HPP

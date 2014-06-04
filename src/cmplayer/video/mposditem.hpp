#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

template<class T> class VideoImageCache;
class MpOsdBitmap;                      class OpenGLTexture2D;

class MpOsdItem {
    using Cache = VideoImageCache<MpOsdBitmap>;
public:
    MpOsdItem();
    ~MpOsdItem();
    auto draw(const Cache &cache) -> bool;
    auto texture() const -> const OpenGLTexture2D&;
    auto initialize() -> void;
    auto finalize() -> void;
    auto isVisible() const noexcept -> bool;
private:
    struct Data;
    Data *d;
};

#endif // MPOSDITEM_HPP

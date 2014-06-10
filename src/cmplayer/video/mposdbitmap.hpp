#ifndef MPOSDBITMAP_HPP
#define MPOSDBITMAP_HPP

struct sub_bitmaps;

class MpOsdBitmap {
public:
    struct Id {
        Id() noexcept { }
        Id(const sub_bitmaps *bitmaps) noexcept;
        auto operator == (const Id &rhs) const noexcept -> bool
            { return image == rhs.image && pos == rhs.pos; }
        auto operator != (const Id &rhs) const noexcept -> bool
            { return !operator == (rhs); }
        int image = -1, pos = -1;
    };
    enum Format {
        NoFormat = 0,
        Ass      = 0x10,
        Rgba     = 0x20, /*Premultipled Alpha*/
    };
    struct PartInfo {
        const QRect &display() const { return m_display; }
        const QSize &size() const { return m_size; }
        const QPoint &map() const { return m_map; }
        auto color() const -> quint32 { return m_color; }
        auto strideAsPixel() const -> int { return m_strideAsPixel; }
        auto width() const -> int { return size().width(); }
        auto height() const -> int { return size().height(); }
    private:
        friend class MpOsdBitmap;
        QRect m_display = {0, 0, 0, 0};
        QSize m_size = {0, 0};
        QPoint m_map = {0, 0};
        quint32 m_color = 0;
        int m_stride = 0, m_offset = 0, m_strideAsPixel = 0;
    };
    auto operator == (const MpOsdBitmap &rhs) const -> bool
        { return m_id == rhs.m_id; }
    auto operator != (const MpOsdBitmap &rhs) const -> bool
        { return !operator == (rhs); }
    auto copy(const sub_bitmaps *imgs, const QSize &renderSize) -> bool;
    template<class T = uchar>
    auto data(int i) -> T*
        { return reinterpret_cast<T*>(m_data.data() + m_parts[i].m_offset); }
    template<class T = uchar>
    auto data(int i) const -> const T*
        { return reinterpret_cast<const T*>(m_data.data() + m_parts[i].m_offset); }
    auto count() const -> int { return m_count; }
    auto part(int i) const -> const PartInfo& { return m_parts[i]; }
    auto format() const -> Format { return m_format; }
    auto renderSize() const -> const QSize& { return m_renderSize; }
    auto sheet() const -> const QSize& { return m_sheet; }
    auto drawOn(QImage &frame) const -> void;
    auto toImage() const -> QImage;
    auto id() const noexcept -> const Id& { return m_id; }
private:
    Id m_id;
    QByteArray m_data;
    int m_count = 0;
    QVector<PartInfo> m_parts;
    Format m_format = Rgba;
    QSize m_sheet = {0, 0}, m_maximumSize = {0, 0}, m_renderSize = {0, 0};
};

#endif // MPOSDBITMAP_HPP

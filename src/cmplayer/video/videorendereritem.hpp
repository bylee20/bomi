#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "quick/highqualitytextureitem.hpp"
#include "enum/videoeffect.hpp"

class VideoColor;                       class Kernel3x3;
enum class DeintMethod;                 enum class ColorRange;
enum class ColorSpace;                  class DeintOption;

template<class T> class VideoImageCache;
class VideoTexture;                     class MpOsdBitmap;
class VideoData;                        class VideoFormat;

class VideoRenderer : public HighQualityTextureItem {
    Q_OBJECT
public:
    using Cache = VideoImageCache<VideoTexture>;
    using OsdCache = VideoImageCache<MpOsdBitmap>;
    VideoRenderer(QQuickItem *parent = 0);
    ~VideoRenderer();
    auto screenRect() const -> QRectF;
    auto offset() const -> QPoint;
    auto aspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> int;
    auto effects() const -> VideoEffects;
    auto sizeHint() const -> QSize;
    auto deintOptionForSwdec() const -> DeintOption;
    auto deintOptionForHwdec() const -> DeintOption;
    auto setAspectRatio(double ratio) -> void;
    auto setDeintOptions(DeintOption swdec, DeintOption hwdec) -> void;
    auto setOverlay(GeometryItem *overlay) -> void;
    auto overlay() const -> QQuickItem*;
    auto present(const VideoData &data) -> void;
    auto prepare(const VideoFormat &format) -> void;
    auto hasFrame() const -> bool;
    auto requestFrameImage() const -> void;
    auto frameRect(const QRectF &area) const -> QRectF;
    auto setKernel(const Kernel3x3 &blur, const Kernel3x3 &sharpen) -> void;
    auto setOverlayOnLetterbox(bool letterbox) -> void;
    auto overlayOnLetterbox() const -> bool;
    auto mapToVideo(const QPointF &pos) -> QPointF;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto isOpaque() const -> bool override { return true; }
    auto setAlignment(int alignment) -> void;
    auto setEffects(VideoEffects effects) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto kernel() const -> const Kernel3x3&;
    auto osdSize() const -> QSize;

    auto equalizer() const -> const VideoColor&;
    auto setEqualizer(const VideoColor &prop) -> void;
    auto setColorRange(ColorRange range) -> void;
    auto setColorSpace(ColorSpace space) -> void;
    auto colorRange() const -> ColorRange;
    auto colorSpace() const -> ColorSpace;
    auto colorMatrixSpace() const -> ColorSpace;
    auto colorMatrixRange() const -> ColorRange;
    auto setChromaUpscaler(InterpolatorType tpe) -> void;
    auto chromaUpscaler() const -> InterpolatorType;

    auto droppedFrames() const -> int;
    auto delayedFrames() const -> int;
    auto fps() const -> double;
    auto resetTimings() -> void;
signals:
    void transferred();
    void frameImageObtained(const QImage &video, const QImage &osd) const;
    void effectsChanged(VideoEffects effects);
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void frameRectChanged(const QRectF &rect);
    void kernelChanged(const Kernel3x3 &kernel);
    void osdSizeChanged(const QSize &size);

    void colorRangeChanged(ColorRange range);
    void colorSpaceChanged(ColorSpace space);
    void chromaUpscalerChanged(InterpolatorType type);
    void equalizerChanged(const VideoColor &eq);
    void overlayOnLetterboxChanged(bool on);
    void alignmentChanged(int alignment);
    void formatChanged(const VideoFormat &format);
    void droppedFramesChanged(int dropped);
    void delayedFramesChanged(int delayed);
    void deintMethodChanged(DeintMethod method);
    void fpsChanged(double fps);
    void colorMatrixChanged();
private:
    auto afterUpdate() -> void;
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto customEvent(QEvent *event) -> void override;
    auto updatePolish() -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    struct Data;
    Data *d;
};

#endif // VIDEORENDERERITEM_HPP

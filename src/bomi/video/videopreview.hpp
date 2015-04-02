#ifndef VIDEOPREVIEW_HPP
#define VIDEOPREVIEW_HPP


#include "quick/simplefboitem.hpp"

class VideoPreview : public SimpleFboItem {
    Q_OBJECT
    using Super = SimpleFboItem;
    Q_PROPERTY(qreal rate READ rate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(QSize sizeHint READ sizeHint NOTIFY sizeHintChanged)
    Q_PROPERTY(qreal aspectRatio READ aspectRatio NOTIFY aspectRatioChanged)
public:
    VideoPreview(QQuickItem *parent = 0);
    ~VideoPreview();
    auto shutdown() -> void;
    auto unload() -> void;
    auto load(const QString &path) -> void;
    auto setRate(double rate) -> void;
    auto rate() const -> double;
    auto sizeHint() const -> QSize;
    auto setSizeHint(const QSize &size) -> void;
    auto hasFrame() const -> bool;
    auto aspectRatio() const -> double;
    auto imageSize() const -> QSize final { return size().toSize(); }
    auto setActive(bool active) -> void;
signals:
    void rateChanged(double rate);
    void sizeHintChanged();
    void aspectRatioChanged();
private:
    auto paint(OpenGLFramebufferObject *fbo) -> void final;
    auto initializeGL() -> void final;
    auto finalizeGL() -> void final;
    auto customEvent(QEvent *event) -> void final;
    struct Data;
    Data *d;
};

#endif // VIDEOPREVIEW_HPP

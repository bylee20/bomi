#include "snapshotdialog.hpp"
#include "ui_snapshotdialog.h"
#include "player/info.hpp"
#include "subtitle/subtitlerendereritem.hpp"
#include "video/videorendereritem.hpp"

struct SnapshotDialog::Data {
    Ui::SnapshotDialog ui;
    const VideoRendererItem *video = nullptr;
    const SubtitleRendererItem *subtitle = nullptr;
    QImage image, sub;
    QRectF subRect;
    bool hasSubtitle = false;
};

SnapshotDialog::SnapshotDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->ui.setupUi(this);
    connect(d->ui.zoomIn, &QAbstractButton::clicked, this, [this] () { d->ui.viewer->scale(1.25); });
    connect(d->ui.zoomOut, &QAbstractButton::clicked, this, [this] () { d->ui.viewer->scale(0.8); });
    connect(d->ui.original, &QAbstractButton::clicked, this, [this] () { d->ui.viewer->scale(1.0); });
    connect(d->ui.take, &QAbstractButton::clicked, this, &SnapshotDialog::take);
    connect(d->ui.subtitle, &QAbstractButton::toggled, this, &SnapshotDialog::updateSnapshot);
    connect(d->ui.clip, &QAbstractButton::clicked, this, [this] () { qApp->clipboard()->setPixmap(d->ui.viewer->image()); });
    connect(d->ui.save, &QAbstractButton::clicked, this, [this] () {
        const auto ext = Info::writableImageExt();
        const QString fileName = _L("cmplayer-snapshot-") % QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") % _L(".png");
        QString file = _GetSaveFileName(this, tr("Save File"), fileName, Info::writableImageExtFilter());
        if (!file.isEmpty()) {
            if (!ext.contains(QFileInfo(file).suffix()))
                file += ".png";
            d->ui.viewer->image().save(file);
        }
    });
}

SnapshotDialog::~SnapshotDialog() {
    delete d;
}

auto SnapshotDialog::setVideoRenderer(const VideoRendererItem *video) -> void
{
    if (d->video)
        disconnect(d->video, 0, this, 0);
    if ((d->video = video))
        connect(d->video, &VideoRendererItem::frameImageObtained, this, [this] (QImage image) {
            const QImage frame = image;
            if (frame.size() == d->video->sizeHint())
                d->image = frame;
            else {
                d->image = QImage(d->video->sizeHint(), QImage::Format_ARGB32_Premultiplied);
                QPainter painter(&d->image);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);
                painter.drawImage(d->video->frameRect(d->image.rect()), frame);
            }
            updateSubtitleImage();
            updateSnapshot(d->ui.subtitle->isChecked());
            d->ui.take->setEnabled(true);
        }, Qt::QueuedConnection);
}

auto SnapshotDialog::updateSubtitleImage() -> void
{
    d->hasSubtitle = false;
    if (d->subtitle) {
        d->sub = d->subtitle->draw(d->image.rect(), &d->subRect);
        d->hasSubtitle = !d->sub.isNull();
    }
}

auto SnapshotDialog::setSubtitleRenderer(const SubtitleRendererItem *subtitle) -> void
{
    d->subtitle = subtitle;
}

auto SnapshotDialog::updateSnapshot(bool sub) -> void
{
    if (!d->video || d->image.isNull()) {
        d->ui.viewer->setText(tr("Failed in getting a snapshot!"));
    } else {
        QPixmap pixmap = QPixmap::fromImage(d->image);
        if (sub && d->hasSubtitle) {
            QPainter painter(&pixmap);
            painter.drawImage(d->subRect, d->sub);
        }
        d->ui.viewer->setImage(pixmap);
    }
    d->ui.save->setEnabled(!d->image.isNull());
}

auto SnapshotDialog::take() -> void
{
    if (!d->video || !d->video->hasFrame())
        return;
    d->ui.take->setEnabled(false);
    d->video->requestFrameImage();
}

struct ImageViewer::Data {
    QLabel *label;
    double scale;
};

ImageViewer::ImageViewer(QWidget *parent)
: QScrollArea(parent), d(new Data) {
    d->scale = 1.0;
    d->label = new QLabel(this);
    d->label->setBackgroundRole(QPalette::Base);
    d->label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    d->label->setScaledContents(true);
    d->label->setMinimumSize(200, 150);
    d->label->setAlignment(Qt::AlignCenter);

    setBackgroundRole(QPalette::Dark);
    setWidget(d->label);
}

ImageViewer::~ImageViewer() {
    delete d;
}

auto ImageViewer::sizeHint() const -> QSize
{
    return d->label->sizeHint() + QSize(5, 5);
}

auto ImageViewer::setText(const QString &text) -> void
{
    d->label->setText(text);
}

auto ImageViewer::setImage(const QPixmap &image) -> void
{
    d->label->setPixmap(image);
    zoomOriginal();
}

auto ImageViewer::zoomOriginal() -> void
{
    d->label->adjustSize();
    d->scale = 1.0;
    emit scaleChanged(d->scale);
}

auto ImageViewer::scale(double factor) -> void
{
    if (qAbs(1.0 - factor) < 1.0e-5)
        zoomOriginal();
    else if (d->label->pixmap()) {
        d->scale *= factor;
        d->label->resize(d->scale * d->label->pixmap()->size());
        adjustScrollBar(horizontalScrollBar(), factor);
        adjustScrollBar(verticalScrollBar(), factor);
        emit scaleChanged(d->scale);
    }
}

auto ImageViewer::adjustScrollBar(QScrollBar *scrollBar, double factor) -> void
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}

auto ImageViewer::image() const -> QPixmap
{
    return d->label->pixmap() ? *d->label->pixmap() : QPixmap();
}

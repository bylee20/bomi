#include "snapshotdialog.hpp"
#include "ui_snapshotdialog.h"

struct SnapshotDialog::Data {
    SnapshotDialog *p = nullptr;
    Ui::SnapshotDialog ui;
    QImage video, sub;
    QRectF subRect;
    auto updateSnapshot(bool showSub) -> void
    {
        if (video.isNull())
            ui.viewer->setText(tr("Failed in getting a snapshot!"));
        else {
            auto pixmap = QPixmap::fromImage(video);
            if (showSub && !sub.isNull()) {
                QPainter painter(&pixmap);
                painter.drawImage(subRect, sub);
            }
            ui.viewer->setImage(pixmap);
        }
        ui.save->setEnabled(!video.isNull());
        if (!p->isVisible()) {
            if (!video.isNull())
                p->resize(video.size() + QSize(40, 60));
            p->show();
        }
    }
};

SnapshotDialog::SnapshotDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->ui.setupUi(this);
    connect(d->ui.zoomIn, &QAbstractButton::clicked,
            this, [this] () { d->ui.viewer->scale(1.25); });
    connect(d->ui.zoomOut, &QAbstractButton::clicked,
            this, [this] () { d->ui.viewer->scale(0.8); });
    connect(d->ui.original, &QAbstractButton::clicked,
            this, [this] () { d->ui.viewer->scale(1.0); });
    connect(d->ui.take, &QAbstractButton::clicked, this, &SnapshotDialog::take);
    connect(d->ui.subtitle, &QAbstractButton::toggled,
            this, [this] (bool sub) { d->updateSnapshot(sub); });
    connect(d->ui.clip, &QAbstractButton::clicked, this,
            [this] () { qApp->clipboard()->setPixmap(d->ui.viewer->image()); });
    connect(d->ui.save, &QAbstractButton::clicked, this, [this] () {
        const auto time = QDateTime::currentDateTime();
        const QString fileName = "cmplayer-snapshot-"_a
                                 % time.toString("yyyy-MM-dd-hh-mm-ss")
                                 % ".png"_a;
        const auto file = _GetSaveFile(this, tr("Save File"),
                                           fileName, ImageExt);
        if (!file.isEmpty())
            d->ui.viewer->image().save(file);
    });
}

SnapshotDialog::~SnapshotDialog() {
    delete d;
}

auto SnapshotDialog::setImage(const QImage &video, const QImage &sub,
                              const QRectF &subRect) -> void
{
    if (video.isNull())
        clear();
    else {
        d->video = video;
        d->sub = sub;
        d->subRect = subRect;
        d->updateSnapshot(d->ui.subtitle->isChecked());
        d->ui.take->setEnabled(true);
    }
}

auto SnapshotDialog::clear() -> void
{
    d->video = d->sub = QImage();
    d->subRect = QRectF();
    d->ui.take->setEnabled(true);
    d->updateSnapshot(false);
}

auto SnapshotDialog::take() -> void
{
    d->ui.take->setEnabled(false);
    emit request();
}

/******************************************************************************/

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

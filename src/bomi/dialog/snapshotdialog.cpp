#include "snapshotdialog.hpp"
#include "ui_snapshotdialog.h"
#include <QClipboard>
#include <QScrollBar>
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(Snapshot)

SnapshotSaver::SnapshotSaver(const QImage &image, const QString &fileName, int quality)
    : m_image(image), m_fileName(fileName), m_quality(quality)
{
    QFile file(fileName);
    m_writable = file.open(QFile::Truncate | QFile::WriteOnly) && file.isWritable();
}

auto SnapshotSaver::run() -> void
{
    if (!m_writable)
        _Error("'%%' is not wriable.", m_fileName);
    else if (!m_image.save(m_fileName, nullptr, m_quality))
        _Error("Failed to save '%%'.", m_fileName);
    else
        _Info("'%%' saved.", m_fileName);

}

struct SnapshotDialog::Data {
    SnapshotDialog *p = nullptr;
    Ui::SnapshotDialog ui;
    QImage video, osd;
    Take take;
    auto updateSnapshot(bool showSub) -> void
    {
        auto image = showSub ? osd : video;
        if (image.isNull())
            ui.viewer->setText(tr("Failed in getting a snapshot!"));
        else
            ui.viewer->setImage(QPixmap::fromImage(image));
        ui.save->setEnabled(!image.isNull());
        if (!p->isVisible()) {
            if (!image.isNull())
                p->resize(image.size() + QSize(40, 60));
            p->show();
        }
    }
};

SnapshotDialog::SnapshotDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->ui.setupUi(this);
    _SetWindowTitle(this, tr("Take Snapshot"));

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
        const QString fileName = "bomi-snapshot-"_a
                % time.toString(u"yyyy-MM-dd-hh:mm:ss.zzz"_q) % ".png"_a;
        const auto file = _GetSaveFile(this, tr("Save File"),
                                           fileName, WritableImageExt);
        if (!file.isEmpty())
            d->ui.viewer->image().save(file);
    });
}

SnapshotDialog::~SnapshotDialog() {
    delete d;
}

auto SnapshotDialog::setImage(const QImage &video, const QImage &osd) -> void
{
    if (video.isNull())
        clear();
    else {
        d->video = video;
        d->osd = osd;
        d->updateSnapshot(d->ui.subtitle->isChecked());
        d->ui.take->setEnabled(true);
    }
}

auto SnapshotDialog::clear() -> void
{
    d->video = d->osd = QImage();
    d->ui.take->setEnabled(true);
    d->updateSnapshot(false);
}

auto SnapshotDialog::take() -> void
{
    d->ui.take->setEnabled(false);
    if (d->take)
        d->take();
}

auto SnapshotDialog::setTakeFunc(Take &&func) -> void
{
    d->take = std::move(func);
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

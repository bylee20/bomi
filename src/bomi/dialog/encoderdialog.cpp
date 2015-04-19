#include "encoderdialog.hpp"
#include "dialog/mbox.hpp"
#include "player/mpv.hpp"
#include "misc/filenamegenerator.hpp"
#include "ui_encoderdialog.h"
#include "misc/objectstorage.hpp"
#include "misc/dataevent.hpp"
#include <QCloseEvent>
extern "C" {
#include <libavcodec/avcodec.h>
}

static constexpr int TickEvent = QEvent::User + 1;

enum FrameRate { CFRAuto, CFRManul, VFR };

static auto allCodecs() -> QPair<QStringList, QStringList>
{
    static const auto list = [] () {
        QPair<QStringList, QStringList> list;
        AVCodec *c = nullptr;
        while ((c = av_codec_next(c))) {
            if (!av_codec_is_encoder(c))
                continue;
            if (c->type == AVMEDIA_TYPE_VIDEO)
                list.first.push_back(_L(c->name));
            else if (c->type == AVMEDIA_TYPE_AUDIO)
                list.second.push_back(_L(c->name));
        }
        av_free(c);
        return list;
    }();
    return list;
}

struct EncoderDialog::Data {
    EncoderDialog *p = nullptr;
    Ui::EncoderDialog ui;
    QSharedPointer<Mpv> mpv;
    QByteArray source, audio;
    QSize size;
    QPushButton *start = nullptr;
    FileNameGenerator g;
    ObjectStorage storage;
    int tick = -1;
    int error = MPV_ERROR_SUCCESS;
    bool resizing = false;
    auto aspect() const -> double
        { return size.isEmpty() ? 1.0 : size.width() / (double)size.height(); }
};

EncoderDialog::EncoderDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    d->p = this;
    d->ui.setupUi(this);

    d->storage.setObject(this, u"encoder-dialog"_q);
    d->storage.add("vc", d->ui.vc, "currentText");
    d->storage.add("ac", d->ui.ac, "currentText");
    d->storage.add("fps", d->ui.fps, "currentIndex");
    d->storage.add(d->ui.fps_value);
//    d->storage.add(d->ui.size);
//    d->storage.add(d->ui.width);
//    d->storage.add(d->ui.height);
    d->storage.add(d->ui.vcopts);
    d->storage.add(d->ui.acopts);
    d->storage.add(d->ui.file);
    d->storage.add(d->ui.folder);
    d->storage.add("ext", d->ui.ext, "currentText");

    d->start = d->ui.bbox->addButton(tr("Start"), BBox::ActionRole, this, [=] () { start(); });
    connect(d->ui.bbox->button(BBox::Close), &QAbstractButton::clicked,
            this, &EncoderDialog::close);
    connect(SIGNAL_VT(d->ui.fps, currentIndexChanged, int), this,
            [=] (int idx) { d->ui.fps_value->setEnabled(idx == CFRManul); });
    connect(SIGNAL_VT(d->ui.ext, currentIndexChanged, int), this, [=] () {
        const auto gif = d->ui.ext->currentText() == u"gif"_q;
        d->ui.vc->setEnabled(!gif);
        d->ui.vcopts->setEnabled(!gif);
        d->ui.ac->setEnabled(!gif);
        d->ui.acopts->setEnabled(!gif);
    });
    connect(SIGNAL_VT(d->ui.width, valueChanged, int), this, [=] (int w) {
        if (d->ui.size->isChecked() && !d->resizing) {
            d->resizing = true;
            d->ui.height->setValue(w / d->aspect() + 0.5);
            d->resizing = false;
        }
    });
    connect(SIGNAL_VT(d->ui.height, valueChanged, int), this, [=] (int h) {
        if (d->ui.size->isChecked() && !d->resizing) {
            d->resizing = true;
            d->ui.width->setValue(h * d->aspect() + 0.5);
            d->resizing = false;
        }
    });
    d->ui.file->setToolTip(FileNameGenerator::toolTip());
    d->ui.browse->setKey(u"encoder-folder"_q);
    d->ui.browse->setEditor(d->ui.folder);
    d->ui.size->setChecked(true);

    const auto codecs = allCodecs();
    auto setDefault = [&] (QComboBox *c, const QStringList &items, const QStringList &fallbacks)
    {
        c->addItems(items);
        for (auto &fb : fallbacks) {
            if (items.contains(fb)) {
                c->setCurrentText(fb);
                return;
            }
        }
    };

    setDefault(d->ui.vc, codecs.first, { u"libx264"_q, u"mpeg4"_q });
    setDefault(d->ui.ac, codecs.second, { u"libvorbis"_q, u"aac"_q });
    d->ui.ext->addItems({ u"avi"_q, u"mkv"_q, u"mp4"_q });
    if (codecs.first.contains(u"gif"_q))
        d->ui.ext->addItem(u"gif"_q);
    d->ui.ext->setCurrentText(u"mkv"_q);
    d->ui.folder->setText(_WritablePath(Location::Movies));
    d->ui.fps->setCurrentIndex(::CFRManul);
    d->ui.fps_value->setValue(30.0);
    d->ui.file->setText(u"%MEDIA_DISPLAY_NAME%-%T_HOUR_0%%T_MIN_0%%T_SEC_0%-%E_HOUR_0%%E_MIN_0%%E_SEC_0%"_q);

    d->storage.restore();
}

EncoderDialog::~EncoderDialog()
{
    cancel();
    d->storage.save();
    delete d;
}

auto EncoderDialog::cancel() -> void
{
    if (d->mpv) {
        if (d->mpv->isRunning()) {
            d->mpv->tellAsync("quit");
            d->mpv->wait(30000);
        }
        d->mpv->destroy();
        d->mpv.clear();
    }
}

auto EncoderDialog::isBusy() const -> bool
{
    return d->mpv;
}

auto EncoderDialog::setSource(const QByteArray &mrl, const QSize &size,
                              const FileNameGenerator &g) -> void
{
    d->source = mrl;
    d->size = size;
    d->resizing = true;
    d->ui.width->setValue(size.width());
    d->ui.height->setValue(size.height());
    d->resizing = false;
    d->audio.clear();
    d->g = g;
    _SetWindowTitle(this, tr("Encoder: %1").arg(d->g.mediaName));
}

auto EncoderDialog::setRange(int start, int end) -> void
{
    d->ui.a->setTime(QTime::fromMSecsSinceStartOfDay(start));
    d->ui.b->setTime(QTime::fromMSecsSinceStartOfDay(end));
}

auto EncoderDialog::setAudio(const QByteArray &audio) -> void
{
    d->audio = audio;
}

auto EncoderDialog::start() -> bool
{
    const auto error = run();
    if (error.isEmpty())
        return true;
    MBox::error(this, tr("Encoder"), error, { BBox::Ok }, BBox::Ok);
    return false;
}

auto EncoderDialog::run() -> QString
{
    if (d->size.isEmpty())
        return tr("No video stream exists.");
    d->g.unix = QDateTime::currentMSecsSinceEpoch();
    d->g.dateTime = QDateTime::currentDateTime();
    d->g.start = d->ui.a->time();
    d->g.end = d->ui.b->time();
    const int a = d->g.start.msecsSinceStartOfDay();
    const int b = d->g.end.msecsSinceStartOfDay();
    if ((b - a) < 100)
        return tr("Range is too short.");
    const auto folder = d->ui.folder->text();
    if (folder.isEmpty() || !QDir(folder).exists())
        return tr("Folder does not exists.");
    if (d->ui.file->text().isEmpty())
        return tr("File name is empty.");
    const auto file = d->g.get(folder, d->ui.file->text(), d->ui.ext->currentText());
    if (file.isEmpty())
        return tr("Failed to create file.");
    Q_ASSERT(!d->mpv);
    d->mpv.reset(new Mpv);
    connect(d->mpv.data(), &QThread::finished, this, [=] () {
        d->mpv->destroy();
        d->mpv.clear();
        d->start->setEnabled(true);
        if (d->error != MPV_ERROR_SUCCESS)
            MBox::error(this, tr("Encoder"), QString::fromUtf8(mpv_error_string(d->error)),
                        { BBox::Ok }, BBox::Ok);
        hide();
    });
    d->mpv->setLogContext("mpv/encoder"_b);
    d->mpv->create();
    d->mpv->setObserver(this);
    d->mpv->observe("time-pos", [=] (double s) { qDebug() << s; });
    d->mpv->request(MPV_EVENT_TICK, [=] (mpv_event*) {
        if (_Change<int>(d->tick, d->mpv->get<double>("time-pos") * 10))
            _PostEvent(this, TickEvent, d->tick);
    });
    d->mpv->request(MPV_EVENT_END_FILE, [=] (mpv_event *e) {
        const auto ev = static_cast<mpv_event_end_file*>(e->data);
        if (ev->reason == MPV_END_FILE_REASON_ERROR)
            d->error = ev->error;
        d->mpv->tellAsync("quit");
    });

    auto _n = [] (auto n) { return QByteArray::number(n); };

    d->mpv->setOption("o", MpvFile(file).toMpv());
    switch (d->ui.fps->currentIndex()) {
    case CFRManul:
        d->mpv->setOption("ofps", _n(d->ui.fps_value->value()));
        break;
    case CFRAuto:
        d->mpv->setOption("oautofps", "yes");
        break;
    }

    const QSize s(d->ui.width->value(), d->ui.height->value());
    if (d->size != s)
        d->mpv->setOption("vf", "scale=" + _n(s.width()) + ':' + _n(s.height()));

    if (d->ui.ext->currentText() == u"gif"_q) {
        d->mpv->setOption("ovc", "gif"_b);
        d->mpv->setOption("aid", "no"_b);
    } else {
        d->mpv->setOption("ovc", d->ui.vc->currentText().toLatin1());
        d->mpv->setOption("oac", d->ui.ac->currentText().toLatin1());
        if (!d->ui.vcopts->text().isEmpty())
            d->mpv->setOption("ovcopts", d->ui.vcopts->text().toUtf8());
        if (!d->ui.acopts->text().isEmpty())
            d->mpv->setOption("oacopts", d->ui.acopts->text().toUtf8());
        if (!d->audio.isEmpty()) {
            bool ok = false;
            d->audio.toInt(&ok);
            if (ok)
                d->mpv->setOption("aid", d->audio);
            else
                d->mpv->setOption("audio-file", d->audio);
        }
    }
    d->mpv->setOption("sid", "no");
    d->mpv->setOption("start", _n(a * 1e-3));
    d->mpv->setOption("end", _n(b * 1e-3));

    d->tick = -1;
    d->error = MPV_ERROR_SUCCESS;
    d->start->setEnabled(false);
    d->ui.prog->setRange(a/100, b/100);
    d->mpv->initialize(Log::Debug, false);
    d->mpv->start();
    d->mpv->tell("loadfile", d->source);
    return QString();
}

auto EncoderDialog::closeEvent(QCloseEvent *event) -> void
{
    if (isBusy() && MBox::ask(this, tr("Encoder"),
                              tr("Do you want to cancel current encoding and close window?"),
                              { BBox::Yes, BBox::Cancel }, BBox::Cancel) == BBox::Cancel) {
        event->setAccepted(false);
        return;
    }
    cancel();
    QDialog::closeEvent(event);
}

auto EncoderDialog::customEvent(QEvent *event) -> void
{
    if (event->type() == TickEvent)
        d->ui.prog->setValue(_GetData<int>(event));
}

#include "encoderdialog.hpp"
#include "dialog/mbox.hpp"
#include "player/mpv.hpp"
#include "misc/filenamegenerator.hpp"
#include "ui_encoderdialog.h"
#include "misc/objectstorage.hpp"
#include "misc/dataevent.hpp"
#include "misc/osdstyle.hpp"
#include "player/streamtrack.hpp"
#include <QCloseEvent>
extern "C" {
#include <libavcodec/avcodec.h>
}

static constexpr int TickEvent = QEvent::User + 1;

enum FrameRate { CFRAuto, CFRManual, VFR };

struct CodecListPair { QStringList ac, vc; };

static auto allCodecs() -> const CodecListPair&
{
    static const auto list = [] () {
        CodecListPair list;
        AVCodec *c = nullptr;
        while ((c = av_codec_next(c))) {
            if (!av_codec_is_encoder(c))
                continue;
            if (c->type == AVMEDIA_TYPE_VIDEO)
                list.vc.push_back(_L(c->name));
            else if (c->type == AVMEDIA_TYPE_AUDIO)
                list.ac.push_back(_L(c->name));
        }
        av_free(c);
        qSort(list.ac);
        qSort(list.vc);
        return list;
    }();
    return list;
}

struct CodecPair { QString ac, vc; };
auto operator << (QDebug dbg, const CodecPair &cp) -> QDebug
{
    return dbg << cp.ac << cp.vc;
}

struct EncoderDialog::Data {
    EncoderDialog *p = nullptr;
    Ui::EncoderDialog ui;
    QSharedPointer<Mpv> mpv;
    QByteArray source;
    StreamTrack audio, sub;
    OsdStyle style;
    QSize size;
    QPushButton *start = nullptr;
    FileNameGenerator g;
    ObjectStorage storage;
    QMap<QString, QVariant> copts;
    QMap<QString, CodecPair> fmts;
    QString ext, ac, vc;
    QRect crop;
    int tick = -1, error = MPV_ERROR_SUCCESS;
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
    d->storage.add("fps", d->ui.fps, "currentIndex");
    d->storage.add(d->ui.fps_value);
    d->storage.add(d->ui.subtitle);
    d->storage.add(d->ui.cx);
    d->storage.add(d->ui.cy);
    d->storage.add(d->ui.cw);
    d->storage.add(d->ui.ch);
    d->storage.add(d->ui.scale);
    d->storage.add("ext", &d->ext);
    d->storage.add("copts", &d->copts);
    d->storage.add("fmts", [=] () -> QVariant {
        QMap<QString, QVariant> var;
        for (auto it = d->fmts.begin(); it != d->fmts.end(); ++it)
            var[it.key()] = QStringList() << it->ac << it->vc;
        return var;
    }, [=] (const QVariant &var) {
        const auto &list = allCodecs();
        const auto map = var.toMap();
        for (auto it = map.begin(); it != map.end(); ++it) {
            const auto strs = it.value().toStringList();
            auto i = d->fmts.find(it.key());
            if (strs.size() != 2 || i == d->fmts.end())
                continue;
            if (std::binary_search(list.ac.begin(), list.ac.end(), strs[0]))
                i->ac = strs[0];
            if (std::binary_search(list.vc.begin(), list.vc.end(), strs[1]))
                i->vc = strs[1];
        }
    });
    d->storage.add(d->ui.file);
    d->storage.add(d->ui.folder);

    d->start = d->ui.bbox->addButton(tr("Start"), BBox::ActionRole, this, [=] () { start(); });
    connect(d->ui.bbox->button(BBox::Close), &QAbstractButton::clicked,
            this, &EncoderDialog::close);
    connect(SIGNAL_VT(d->ui.fps, currentIndexChanged, int), this,
            [=] (int idx) { d->ui.fps_value->setEnabled(idx == CFRManual); });

    d->ui.file->setToolTip(FileNameGenerator::toolTip());
    d->ui.browse->setKey(u"encoder-folder"_q);
    d->ui.browse->setEditor(d->ui.folder);

    d->fmts[u"mkv"_q] = { u"libvorbis"_q, u"libx264"_q };
    d->fmts[u"mp4"_q] = { u"aac"_q, u"libx264"_q };
    d->fmts[u"webm"_q] = { u"libvorbis"_q, u"libvpx"_q };
    d->fmts[u"gif"_q] = { QString(), u"gif"_q };
    d->ext = u"mkv"_q;
    d->copts[u"libvpx"_q] = u"b=1M"_q;

    d->ui.ext->addItems(d->fmts.keys());
    d->ui.ac->addItems(allCodecs().ac);
    d->ui.vc->addItems(allCodecs().vc);
    d->ui.folder->setText(_WritablePath(Location::Movies));
    d->ui.fps->setCurrentIndex(::CFRAuto);
    d->ui.fps_value->setValue(30.0);
    d->ui.file->setText(u"%MEDIA_DISPLAY_NAME%-%T_HOUR_0%%T_MIN_0%%T_SEC_0%-%E_HOUR_0%%E_MIN_0%%E_SEC_0%"_q);

    d->storage.restore();

    d->vc = d->fmts[d->ext].vc;
    d->ac = d->fmts[d->ext].ac;
    d->ui.ext->setCurrentText(d->ext);
    d->ui.vc->setCurrentText(d->vc);
    d->ui.ac->setCurrentText(d->ac);
    d->ui.vcopts->setText(_C(d->copts)[d->vc].toString());
    d->ui.acopts->setText(_C(d->copts)[d->ac].toString());
#define PLUG(av) \
    connect(SIGNAL_VT(d->ui.av##c, currentTextChanged, const QString&), \
            this, [=] (const QString &c) { \
        d->copts[d->av##c] = d->ui.av##copts->text(); \
        d->ui.av##copts->setText(_C(d->copts)[c].toString()); d->av##c = c; });
    PLUG(a); PLUG(v);
#undef PLUG
    connect(SIGNAL_VT(d->ui.ext, currentTextChanged, const QString&), this, [=] (const QString &ext) {
        d->fmts[d->ext] = { d->ui.ac->currentText(), d->ui.vc->currentText() };
        auto &av = d->fmts[d->ext = ext];
        d->ui.ac->setCurrentText(av.ac);
        d->ui.vc->setCurrentText(av.vc);
        const auto gif = ext == u"gif"_q;
        d->ui.vc->setEnabled(!gif);
        d->ui.ac->setEnabled(!gif);
        d->ui.acopts->setEnabled(!gif);
    });

    auto updateResultSize = [=] () {
        auto s = (d->crop & QRect(0, 0, d->size.width(), d->size.height())).size();
        s *= d->ui.scale->value() * 1e-2;
        d->ui.result_size->setText(u" (%1x%2)"_q.arg(s.width()).arg(s.height()));
    };
    connect(SIGNAL_VT(d->ui.cx, valueChanged, int), this, &EncoderDialog::updateCropArea);
    connect(SIGNAL_VT(d->ui.cy, valueChanged, int), this, &EncoderDialog::updateCropArea);
    connect(SIGNAL_VT(d->ui.cw, valueChanged, int), this, &EncoderDialog::updateCropArea);
    connect(SIGNAL_VT(d->ui.ch, valueChanged, int), this, &EncoderDialog::updateCropArea);
    connect(this, &EncoderDialog::cropAreaChanged, this, updateResultSize);
    connect(SIGNAL_VT(d->ui.scale, valueChanged, double), this, updateResultSize);

    updateCropArea();
    updateResultSize();
}

EncoderDialog::~EncoderDialog()
{
    cancel();
    d->ac = d->ui.ac->currentText();
    d->vc = d->ui.vc->currentText();
    d->fmts[d->ext] = { d->ac, d->vc };
    d->copts[d->ac] = d->ui.acopts->text();
    d->copts[d->vc] = d->ui.vcopts->text();
    d->storage.save();
    delete d;
}

auto EncoderDialog::updateCropArea() -> void
{
    const QRect crop(d->ui.cx->value(), d->ui.cy->value(),
                     d->ui.cw->value(), d->ui.ch->value());
    const auto s = (crop & QRect(0, 0, d->size.width(), d->size.height())).size();
    d->ui.cw->setValue(s.width());
    d->ui.ch->setValue(s.height());
    if (_Change(d->crop, crop))
        emit cropAreaChanged(d->crop);
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
    if (_Change(d->source, mrl)) {
        d->ui.a->clear();
        d->ui.b->clear();
    }
    d->size = size;
    d->resizing = true;
    d->ui.cw->setValue(size.width());
    d->ui.ch->setValue(size.height());
    d->resizing = false;
    d->audio = d->sub = StreamTrack();
    d->g = g;
    _SetWindowTitle(this, tr("Encoder: %1").arg(d->g.mediaName));
    updateCropArea();
}

auto EncoderDialog::setRange(int start, int end) -> void
{
    d->ui.a->setTime(QTime::fromMSecsSinceStartOfDay(start));
    d->ui.b->setTime(QTime::fromMSecsSinceStartOfDay(end));
}

auto EncoderDialog::setAudio(const StreamTrack &audio) -> void
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
    d->g.unix_ = QDateTime::currentMSecsSinceEpoch();
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
    case CFRManual:
        d->mpv->setOption("ofps", _n(d->ui.fps_value->value()));
        break;
    case CFRAuto:
        d->mpv->setOption("oautofps", "yes");
        break;
    }

    const QRect orig(0, 0, d->size.width(), d->size.height());
    auto crop = d->crop & orig;
    crop.setWidth((crop.width() / 2) * 2);
    crop.setHeight((crop.height() / 2) * 2);
    QByteArray vf;
    if (crop != orig) {
        vf = "crop=" + _n(crop.width()) + ':' + _n(crop.height())
                + ':' + _n(crop.x())
                + ':' + _n(crop.y());
    }
    const auto size = crop.size() * d->ui.scale->value() * 1e-2;
    if (size != crop.size()) {
        if (!vf.isEmpty())
            vf += ',';
        vf += "scale=" + _n(size.width()) + ':' + _n(size.height());
    }
    if (!vf.isEmpty())
        d->mpv->setOption("vf", vf);

    if (d->ui.ext->currentText() == u"gif"_q) {
        d->mpv->setOption("ovc", "gif"_b);
        if (!d->ui.vcopts->text().isEmpty())
            d->mpv->setOption("ovcopts", d->ui.vcopts->text().toUtf8());
        d->mpv->setOption("aid", "no"_b);
    } else {
        d->mpv->setOption("ovc", d->ui.vc->currentText().toLatin1());
        d->mpv->setOption("oac", d->ui.ac->currentText().toLatin1());
        if (!d->ui.vcopts->text().isEmpty())
            d->mpv->setOption("ovcopts", d->ui.vcopts->text().toUtf8());
        if (!d->ui.acopts->text().isEmpty())
            d->mpv->setOption("oacopts", d->ui.acopts->text().toUtf8());
        if (d->audio.isValid()) {
            if (d->audio.isExternal())
                d->mpv->setOption("audio-file", MpvFile(d->audio.file()).toMpv());
            else
                d->mpv->setOption("aid", _n(d->audio.id()));
        }
    }

    if (!d->ui.subtitle->isChecked())
        d->mpv->setOption("sid", "no");
    else {
        const auto color = [] (const QColor &color) { return color.name(QColor::HexArgb).toLatin1(); };
        const auto &style = d->style;
        const auto &font = style.font;
        d->mpv->setOption("sub-text-color", color(font.color));
        QStringList fontStyles;
        if (font.bold())
            fontStyles.append(u"Bold"_q);
        if (font.italic())
            fontStyles.append(u"Italic"_q);
        QString family = font.family();
        if (!fontStyles.isEmpty())
            family += ":style="_a % fontStyles.join(' '_q);
        const double factor = font.size * 720.0;
        d->mpv->setOption("sub-text-font", family.toUtf8());
        d->mpv->setOption("sub-text-font-size", _n(factor));
        const auto &outline = style.outline;
        const auto scaled = [factor] (double v)
            { return qBound(0., v*factor, 10.); };
        if (outline.enabled) {
            d->mpv->setOption("sub-text-border-size", _n(scaled(outline.width)));
            d->mpv->setOption("sub-text-border-color", color(outline.color));
        } else
            d->mpv->setOption("sub-text-border-size", "0.0");
        const auto &bbox = style.bbox;
        if (bbox.enabled)
            d->mpv->setOption("sub-text-back-color", color(bbox.color));
        else
            d->mpv->setOption("sub-text-back-color", color(Qt::transparent));
        auto norm = [] (const QPointF &p) { return sqrt(p.x()*p.x() + p.y()*p.y()); };
        const auto &shadow = style.shadow;
        if (shadow.enabled) {
            d->mpv->setOption("sub-text-shadow-color", color(shadow.color));
            d->mpv->setOption("sub-text-shadow-offset", _n(scaled(norm(shadow.offset))));
        } else {
            d->mpv->setOption("sub-text-shadow-color", color(Qt::transparent));
            d->mpv->setOption("sub-text-shadow-offset", "0.0");
        }
        // these should be applied?
        //    d->mpv.setAsync("ass-force-margins", d->vr->overlayOnLetterbox() && override); };
        //    d->d->mpv->setAsync("sub-pos", o || !isAss() ? qRound(p * 100) : 100);
        //    d->d->mpv->setAsync("sub-scale", o || !isAss() ? qMax(0., 1. + s) : 1.);
        //    d->d->mpv->setAsync("ass-style-override", o ? "force"_b : "yes"_b);
        if (d->sub.isValid()) {
            if (d->sub.isExternal()) {
                d->mpv->setOption("sub-file", MpvFile(d->sub.file()).toMpv());
                auto cp = d->sub.encoding().name().replace("Windows-"_a, "cp"_a, Qt::CaseInsensitive);
                d->mpv->setOption("subcp", cp.toLatin1());
            } else
                d->mpv->setOption("sid", _n(d->sub.id()));
        }
    }

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

auto EncoderDialog::setSubtitle(const StreamTrack &sub, const OsdStyle &style) -> void
{
    d->sub = sub;
    d->style = style;
}

auto EncoderDialog::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    emit visibleChanged(true);
}

auto EncoderDialog::hideEvent(QHideEvent *event) -> void
{
    QDialog::hideEvent(event);
    emit visibleChanged(false);
}

auto EncoderDialog::cropArea() const -> QRect
{
    return d->crop;
}

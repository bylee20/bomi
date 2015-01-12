#include "deintwidget.hpp"
#include "datacombobox.hpp"
#include "video/deintcaps.hpp"

static constexpr auto GPU = DeintDevice::GPU;
static constexpr auto CPU = DeintDevice::CPU;
static constexpr auto OpenGL = DeintDevice::OpenGL;

struct DeintWidget::Data {
    bool updating = false, hwdec = false;
    DataComboBox *combo = nullptr;
    QCheckBox *gl = nullptr, *doubler = nullptr, *gpu = nullptr;
    QMap<DeintMethod, DeintCaps> caps;
    QList<DeintCaps> defaults;
    DecoderDevice decoder;
    auto current() -> DeintCaps&
        { return caps[(DeintMethod)combo->currentData().toInt()]; }
};

DeintWidget::DeintWidget(DecoderDevice decoder, QWidget *parent)
    : QWidget(parent)
    , d(new Data)
{
    d->decoder = decoder;
    d->hwdec = decoder == DecoderDevice::GPU;
    QSettings r;
    r.beginGroup(u"deint_caps"_q);
    const auto tokens = r.value(DecoderDeviceInfo::name(decoder));
    r.endGroup();
    for (const auto &token : tokens.toStringList()) {
        auto caps = DeintCaps::fromString(token);
        if (caps.isAvailable())
            d->caps[caps.method()] = caps;
    }
    d->combo = new DataComboBox(this);
    d->defaults = DeintCaps::list();
    for (auto &caps : d->defaults) {
        if (!caps.isAvailable())
            continue;
        if ((d->hwdec && !caps.hwdec()) || (!d->hwdec && !caps.swdec()))
            continue;
        const auto method = caps.method();
        d->combo->addItem(DeintMethodInfo::name(method), (int)method);
        if (!d->caps.contains(method))
            d->caps[method] = caps;
        d->caps[method].m_decoders = d->decoder;
    }
    d->doubler = new QCheckBox(tr("Double framerate"), this);
    d->gl = new QCheckBox(tr("Use OpenGL"), this);
    d->gpu = new QCheckBox(tr("Use hardware acceleration if available"), this);
    auto hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("Method"), this));
    hbox->addWidget(d->combo);
    hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    auto vbox = new QVBoxLayout;
    vbox->addLayout(hbox);
    vbox->addWidget(d->doubler);
    vbox->addWidget(d->gl);
    vbox->addWidget(d->gpu);
    setLayout(vbox);

    auto update = [this] (DeintMethod method) {
        d->updating = true;
        auto &cap = d->caps[method];
        const auto &def = d->defaults[(int)method];
        const auto dev = d->hwdec ? GPU : CPU;
        d->gpu->setEnabled(d->hwdec && def.supports(GPU));
        d->gpu->setChecked(d->hwdec && cap.supports(GPU));
        d->gl->setEnabled(def.supports(dev) && def.supports(OpenGL));
        d->gl->setChecked(!def.supports(dev) || cap.supports(OpenGL));
        d->doubler->setEnabled(def.doubler());
        d->doubler->setChecked(cap.doubler());
        d->updating = false;
        cap.m_devices = def.m_devices;
        if (!d->gl->isChecked())
            cap.m_devices &= ~OpenGL;
        if (!d->gpu->isChecked())
            cap.m_devices &= ~GPU;
    };
    connect(d->combo, &DataComboBox::currentDataChanged,
            [update] (const QVariant &data) {
        update(DeintMethod(data.toInt()));
    });
    connect(d->doubler, &QCheckBox::toggled, [this] (bool on) {
        if (!d->updating) d->current().m_doubler = on;
    });
    connect(d->gl, &QCheckBox::toggled, [this] (bool on) {
        if (!d->updating)
            d->current().m_devices.set(OpenGL, on);
    });
    connect(d->gpu, &QCheckBox::toggled, [this] (bool on) {
        if (!d->updating)
            d->current().m_devices.set(GPU, on);
    });
    update(DeintMethod::Bob);
}

DeintWidget::~DeintWidget() {
    QStringList tokens;
    for (auto it = d->caps.begin(); it != d->caps.end(); ++it)
        tokens.push_back(it->toString());
    QSettings r;
    r.beginGroup(u"deint_caps"_q);
    r.setValue(DecoderDeviceInfo::name(d->decoder), tokens);
    r.endGroup();
    delete d;
}

auto DeintWidget::set(const DeintCaps &caps) -> void
{
    (d->caps[caps.method()] = caps).m_decoders = d->decoder;
    d->combo->setCurrentData((int)caps.method());
}

auto DeintWidget::get() const -> DeintCaps
{
    return d->current();
}

auto DeintWidget::informations() -> QString
{
    auto methodText = [] (DeintMethod method, const QString &desc) -> QString
        { return DeintMethodInfo::name(method) % ": "_a % desc; };
    QString text =
        '\n'_q % tr("Methods") % "\n\n"_a %
        methodText(DeintMethod::Bob,
                   tr("Display each line twice.")) % '\n'_q %
        methodText(DeintMethod::LinearBob,
                   tr("Bob with linear interpolation.")) % '\n'_q %
        methodText(DeintMethod::CubicBob,
                   tr("Bob with cubic interpolation.")) % '\n'_q %
        methodText(DeintMethod::LinearBlend,
                   tr("Blend linearly each line with (1 2 1) filter."))%'\n'_q %
        methodText(DeintMethod::Median,
                   tr("Apply median filter to every second line.")) % '\n'_q %
        methodText(DeintMethod::Yadif,
                   tr("Use complicated temporal and spatial interpolation."))
                   % "\n\n"_a %
        tr("Double framerate") % "\n\n"_a %
        tr("This option makes the framerate doubled. "
           "You can get smoother and fluid motions "
           "but it requires more CPU or GPU usage.") % "\n\n"_a %
        tr("Use OpenGL") % "\n\n"_a %
        tr("In most case, deinterlacing with OpenGL can be performed faster "
           "unless your graphics driver has poor support of OpenGL.") % "\n\n"_a %
        tr("Use hardware acceleration if available") % "\n\n"_a %
        tr("Some methods can be accelerated with GPU "
           "by turning on this option if your hardware supports VA-API well.")
    ;
    return text;
}

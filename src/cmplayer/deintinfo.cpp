#include "deintinfo.hpp"
#include "widgets.hpp"
#include "record.hpp"
#include "hwacc.hpp"

QList<DeintCaps> DeintCaps::list() {
	static QList<DeintCaps> caps;
	if (!caps.isEmpty())
		return caps;
	for (int i=0; i<DeintMethodInfo::size(); ++i) {
		caps.push_back(DeintCaps());
		caps.back().m_method = (DeintMethod)i;
	}

	auto set = [] (DeintMethod method, bool cpu, bool gl, bool doubler) {
		auto &cap = caps[(int)method];
		cap.m_method = method;
		if (cpu) {
			cap.m_device |= DeintDevice::CPU;
			cap.m_decoder |= DecoderDevice::CPU;
		}
		if (gl) {
			cap.m_device |= DeintDevice::OpenGL;
			cap.m_decoder |= (DecoderDevice::CPU | DecoderDevice::GPU);
		}
		if (HwAcc::supports(method)) {
			cap.m_device |= DeintDevice::GPU;
			cap.m_decoder |= DecoderDevice::GPU;
		}
		cap.m_doubler = doubler;
		return caps[(int)method];
	};
	//  method                       cpu    gl     doubler
	set(DeintMethod::Bob           , false, true , true );
	set(DeintMethod::LinearBob     , true , true , true );
	set(DeintMethod::CubicBob      , true , false, true );
	set(DeintMethod::LinearBlend   , true , false, false);
	set(DeintMethod::Yadif         , true , false, true );
	set(DeintMethod::Median        , true , false, true );
	set(DeintMethod::MotionAdaptive, false, false, true );
	return caps;
}

DeintCaps DeintCaps::default_(DecoderDevice dec) {
	auto ref = list()[(int)DeintMethod::Bob];
	Q_ASSERT(ref.m_decoder & dec);
	DeintCaps caps;
	caps.m_doubler = ref.m_doubler;
	caps.m_decoder = (int)dec;
	caps.m_device = (int)((dec == DecoderDevice::CPU) ? DeintDevice::CPU : DeintDevice::GPU);
	return caps;
}

QString DeintCaps::toString() const {
	QString text = DeintMethodInfo::name(m_method) % _L('|');
	for (auto dec : DecoderDeviceInfo::items()) {
		if (dec.value & m_decoder)
			text += dec.name % _L(':');
	}
	text += "|";
	for (auto dev : DeintDeviceInfo::items()) {
		if (dev.value & m_device)
			text += dev.name % _L(':');
	}
	text += "|" % _N(m_doubler);
	return text;
}

DeintCaps DeintCaps::fromString(const QString &text) {
	auto tokens = text.split('|', QString::SkipEmptyParts);
	if (tokens.size() != 4)
		return DeintCaps();
	DeintCaps caps;
	caps.m_method = DeintMethodInfo::from(tokens[0]);
	for (auto dec : tokens[1].split(':', QString::SkipEmptyParts))
		caps.m_decoder |= DecoderDeviceInfo::from(dec);
	for (auto dev : tokens[2].split(':', QString::SkipEmptyParts))
		caps.m_device |= DeintDeviceInfo::from(dev);
	caps.m_doubler = tokens[3].toInt();
	return caps;
}

QString DeintOption::toString() const {
	return DeintMethodInfo::name(method) % "|" % _N(doubler) % "|" % DeintDeviceInfo::name(device);
}

DeintOption DeintOption::fromString(const QString &string) {
	QStringList tokens = string.split("|", QString::SkipEmptyParts);
	if (tokens.size() != 3)
		return DeintOption();
	DeintOption opt;
	opt.method = DeintMethodInfo::from(tokens[0]);
	opt.doubler = tokens[1].toInt();
	opt.device = DeintDeviceInfo::from(tokens[2]);
	return opt;
}

struct DeintWidget::Data {
	DataComboBox *combo = nullptr;
	QCheckBox *gl = nullptr, *doubler = nullptr, *gpu = nullptr;
	QMap<DeintMethod, DeintCaps> caps;
	QList<DeintCaps> defaults;
	bool updating = false;
	bool hwdec = false;
	DecoderDevice decoder;
	DeintCaps &current() { return caps[(DeintMethod)combo->currentData().toInt()]; }
};

DeintWidget::DeintWidget(DecoderDevice decoder, QWidget *parent)
: QWidget(parent), d(new Data) {
	d->decoder = decoder;
	d->hwdec = decoder == DecoderDevice::GPU;
	Record r("deint_caps");
	const auto tokens = r.value(DecoderDeviceInfo::name(decoder)).toStringList();
	for (const auto &token : tokens) {
		auto caps = DeintCaps::fromString(token);
		if (caps.isAvailable())
			d->caps[caps.method()] = caps;
	}
	d->combo = new DataComboBox(this);
	d->defaults = DeintCaps::list();
	for (auto &caps : d->defaults) {
		if (!caps.isAvailable() || (d->hwdec && !caps.hwdec()) || (!d->hwdec && !caps.swdec()))
			continue;
		const auto method = caps.method();
		d->combo->addItem(DeintMethodInfo::name(method), (int)method);
		if (!d->caps.contains(method))
			d->caps[method] = caps;
		d->caps[method].m_decoder = (int)d->decoder;
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
		cap.m_device = def.m_device;
		if (!d->gl->isChecked())
			cap.m_device &= ~OpenGL;
		if (!d->gpu->isChecked())
			cap.m_device &= ~GPU;
	};
	connect(d->combo, &DataComboBox::currentDataChanged, [update] (const QVariant &data) {
		update(DeintMethod(data.toInt()));
	});
	connect(d->doubler, &QCheckBox::toggled, [this] (bool on) {
		if (!d->updating) d->current().m_doubler = on;
	});
	connect(d->gl, &QCheckBox::toggled, [this] (bool on) {
		if (!d->updating) {
			if (on)
				d->current().m_device |= OpenGL;
			else
				d->current().m_device &= ~OpenGL;
		}
	});
	connect(d->gpu, &QCheckBox::toggled, [this] (bool on) {
		if (!d->updating) {
			if (on)
				d->current().m_device |= GPU;
			else
				d->current().m_device &= ~GPU;
		}
	});
	update(DeintMethod::Bob);
}

DeintWidget::~DeintWidget() {
	Record r("deint_caps");
	QStringList tokens;
	for (auto it = d->caps.begin(); it != d->caps.end(); ++it)
		tokens << it->toString();
	r.write(tokens, DecoderDeviceInfo::name(d->decoder));
	delete d;
}

void DeintWidget::set(const DeintCaps &caps) {
	(d->caps[caps.method()] = caps).m_decoder = (int)d->decoder;
	d->combo->setCurrentData((int)caps.method());
}

DeintCaps DeintWidget::get() const {
	return d->current();
}

QString DeintWidget::informations() {
	auto methodText = [] (DeintMethod method, const QString &desc) -> QString {
		return DeintMethodInfo::name(method) % ": " % desc;
	};
	QString text =
		'\n' % tr("Methods") % "\n\n" %
		methodText(DeintMethod::Bob, tr("Display each line twice.")) % '\n' %
		methodText(DeintMethod::LinearBob, tr("Bob with linear interpolation.")) % '\n' %
		methodText(DeintMethod::CubicBob, tr("Bob with cubic interpolation.")) % '\n' %
		methodText(DeintMethod::LinearBlend, tr("Blend linearly each line with (1 2 1) filter.")) % '\n' %
		methodText(DeintMethod::Median, tr("Apply median filter to every second line.")) % '\n' %
		methodText(DeintMethod::Yadif, tr("Use complicated temporal and spatial interpolation.")) % "\n\n" %
		tr("Double framerate") % "\n\n" %
		tr("This option makes the framerate doubled. You can get smoother and fluid motions but it requires more CPU or GPU usage.") % "\n\n" %
		tr("Use OpenGL") % "\n\n" %
		tr("In most case, deinterlacing with OpenGL can be performed faster unless your graphics driver has poor support of OpenGL.") % "\n\n" %
		tr("Use hardware acceleration if available") % "\n\n" %
		tr("Some methods can be accelerated with GPU by turning on this option if your hardware supports VA-API well.")
	;
	return text;
}

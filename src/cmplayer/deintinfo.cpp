#include "deintinfo.hpp"
#include "widgets.hpp"
#include "record.hpp"

class DeintCaps {
public:
	static QList<DeintCaps> list();
	DeintMethod method() const { return m_method; }
	bool hwdec() const { return m_hwdec; }
	bool swdec() const { return m_swdec; }
	bool cpu() const { return m_cpu; }
	bool gpu() const { return m_gpu; }
	bool doubler() const { return m_doubler; }
private:
	friend class DeintInfo;
	DeintMethod m_method = DeintMethod::None;
	bool m_hwdec = false, m_swdec = false;
	bool m_cpu = false, m_gpu = false;
	bool m_doubler = false;
};

//QString DeintMethod::description(Method method) {
//	QString desc;

//}

QList<DeintCaps> DeintCaps::list() {
	static QList<DeintCaps> caps;
	if (!caps.isEmpty())
		return caps;
	for (int i=0; i<DeintMethodInfo::size(); ++i)
		caps.push_back(DeintCaps());

	auto set = [] (DeintMethod method, bool cpu, bool gpu, bool sw, bool hw, bool doubler) {
		auto &cap = caps[(int)method];
		cap.m_method = method;
		cap.m_cpu = cpu;
		cap.m_gpu = gpu;
		cap.m_swdec = sw;
		cap.m_hwdec = hw;
		cap.m_doubler = doubler;
		return caps[(int)method];
	};
	//  method                    cpu    gpu    swdec  hwdec  doubler
	set(DeintMethod::Bob        , false, true , true , true , true );
	set(DeintMethod::LinearBob  , true , true , true , true , true );
	set(DeintMethod::CubicBob   , true , false, true , false, true );
	set(DeintMethod::LinearBlend, true , false, true , false, false);
	set(DeintMethod::Yadif      , true , false, true , false, true );
	set(DeintMethod::Median     , true , false, true , false, true );
	return caps;
}

QString DeintOption::toString() const {
	return _L(DeintMethodInfo::name(method)) + "|" + _N(doubler) + "|" + _N(hwacc);
}

DeintOption DeintOption::fromString(const QString &string) {
	QStringList tokens = string.split("|", QString::SkipEmptyParts);
	if (tokens.size() != 3)
		return DeintOption();
	DeintOption opt;
	opt.method = DeintMethodInfo::from(tokens[0]);
	opt.doubler = tokens[1].toInt();
	opt.hwacc = tokens[2].toInt();
	return opt;
}

DeintOption DeintOption::default_(DeintMethod method) {
	auto cap = DeintCaps::list()[(int)method];
	DeintOption option;
	option.method = method;
	option.hwacc = cap.gpu();
	option.doubler = cap.doubler();
	return option;
}

struct DeintWidget::Data {
	DataComboBox *combo = nullptr;
	QCheckBox *hwacc = nullptr, *doubler = nullptr;
	QMap<DeintMethod, DeintCaps> caps;
	QMap<DeintMethod, DeintOption> options;
	bool updating = false;
	bool hwdec = false;
	DeintOption &option() { return options[(DeintMethod)combo->currentData().toInt()]; }
};

DeintWidget::DeintWidget(bool hwdec, QWidget *parent)
: QWidget(parent), d(new Data) {
	d->hwdec = hwdec;
	Record r("deint_options");
	QStringList tokens = r.value(hwdec ? "hwdec" : "swdec").toStringList();
	for (QString token : tokens) {
		auto opt = DeintOption::fromString(token);
		if (opt.method != DeintMethod::None)
			d->options[opt.method] = opt;
	}
	d->combo = new DataComboBox(this);
	const auto caps = DeintCaps::list();
	for (auto &cap : caps) {
		if (cap.method() == DeintMethod::None || (hwdec && !cap.hwdec()) || (!hwdec && !cap.swdec()))
			continue;
		const auto method = cap.method();
		d->combo->addItem(DeintMethodInfo::name(method), (int)method);
		d->caps[method] = cap;
		if (!d->options.contains(cap.method()))
			d->options[method] = DeintOption::default_(method);
	}
	d->doubler = new QCheckBox(tr("Make FPS doubled"), this);
	d->hwacc = new QCheckBox(tr("Enable H/W Acc."), this);
	auto hbox = new QHBoxLayout;
	hbox->addWidget(d->combo);
	hbox->addWidget(d->doubler);
	hbox->addWidget(d->hwacc);
	setLayout(hbox);

	auto update = [hwdec, this] (DeintMethod method) {
		d->updating = true;
		const auto &cap = d->caps[method];
		auto &opt = d->options[method];
		d->doubler->setEnabled(cap.doubler());
		d->doubler->setChecked(opt.doubler);
		d->hwacc->setEnabled(!hwdec && cap.gpu() && cap.cpu());
		if (cap.gpu() && cap.cpu())
			d->hwacc->setChecked(opt.hwacc);
		else
			d->hwacc->setChecked(opt.hwacc = cap.gpu());
		d->updating = false;
	};
	connect(d->combo, &DataComboBox::currentDataChanged, [update] (const QVariant &data) {
		update(DeintMethod(data.toInt()));
	});
	connect(d->doubler, &QCheckBox::toggled, [this] (bool on) {
		if (!d->updating) d->option().doubler = on;
	});
	connect(d->hwacc, &QCheckBox::toggled, [this] (bool on) {
		if (!d->updating) d->option().hwacc = on;
	});
	update(DeintMethod::Bob);
}

DeintWidget::~DeintWidget() {
	Record r("deint_options");
	QStringList tokens;
	for (auto it = d->options.begin(); it != d->options.end(); ++it)
		tokens << it->toString();
	r.write(tokens, d->hwdec ? "hwdec" : "swdec");
	delete d;
}

void DeintWidget::setOption(const DeintOption &opt) {
	auto &option = d->options[opt.method];
	option.doubler = opt.doubler;
	option.hwacc = opt.hwacc;
	d->combo->setCurrentData((int)opt.method);
}

DeintOption DeintWidget::option() const {
	return d->option();
}

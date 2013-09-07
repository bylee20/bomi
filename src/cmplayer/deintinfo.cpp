#include "deintinfo.hpp"

struct DeintInfo::Item {
	Method method;
	int caps;
	QString name;
	static const std::array<Item, MethodCount> &list();
};

static QList<DeintInfo> deints_hwdec, deints_swdec;

static void initDeintList() {
	if (!deints_hwdec.isEmpty() && !deints_swdec.isEmpty())
		return;
	const auto deints = DeintInfo::list();
	for (auto &deint : deints) {
		const int caps = deint.capabilities();
		deints_swdec << deint;
		if (caps & DeintInfo::Hardware)
			deints_hwdec << deint;
	}
}

QList<DeintInfo> DeintInfo::hwdecList() { initDeintList(); return deints_hwdec; }
QList<DeintInfo> DeintInfo::swdecList() { initDeintList(); return deints_swdec; }

const std::array<DeintInfo::Item, DeintInfo::MethodCount> &DeintInfo::Item::list() {
	static std::array<DeintInfo::Item, DeintInfo::MethodCount> list;
	static bool first = true;
	if (!first)
		return list;
#define INIT(mtd, cps) {list[mtd].method = mtd; list[mtd].caps = cps; list[mtd].name = #mtd;}
	INIT(Bob, Hardware | SingleRate | DoubleRate);
	INIT(LinearBob, OpenGL | PostProc | SingleRate | DoubleRate);
	INIT(CubicBob, PostProc | SingleRate | DoubleRate);
	INIT(Yadif, AvFilter | SingleRate | DoubleRate);
	INIT(LinearBlend, PostProc | SingleRate);
	INIT(Median, PostProc | SingleRate | DoubleRate);
#undef INIT
	first = false;
	return list;
}

DeintInfo::DeintInfo(Method method, int options)
: m_method(method), m_flags(options & Item::list()[method].caps) {}

QList<DeintInfo> DeintInfo::list() {
	QList<DeintInfo> list;
	list.reserve(MethodCount);
	for (auto &item : Item::list()) {
		DeintInfo info;
		info.m_method = item.method;
		info.m_flags = item.caps;
		list << info;
	}
	return list;
}

DeintInfo DeintInfo::fromString(const QString &text) {
	auto idx = text.indexOf("|");
	if (idx < 0)
		return DeintInfo();
	const int flags = text.mid(idx + 1).toInt();
	const QString name = text.left(idx);
	auto &list = Item::list();
	auto it = _FindIf(list, [name] (const Item &item) { return item.name == name; });
	return it != list.end() ? DeintInfo(it->method, flags) : DeintInfo();
}

int DeintInfo::capabilities() const {
	return Item::list()[m_method].caps;
}

QString DeintInfo::name() const {
	return Item::list()[m_method].name;
}

QString DeintInfo::name(Method method) {
	return Item::list()[method].name;
}

int DeintInfo::capabilities(Method method) {
	return Item::list()[method].caps;
}

QString DeintInfo::description(Method method) {
	QString desc;
	switch (method) {
	case Bob:
		desc = tr("Display each line twice.");
		break;
	case LinearBob:
		desc = tr("Bob with linear interpolation. Deinterlace the given picture by linearly interpolating every second line.");
		break;
	case CubicBob:
		desc = tr("Bob with cubic interpolation. Deinterlace the given block by cubically interpolating every second line.");
		break;
	case Yadif:
		desc = tr("Yet Another DeInterlacing Filter. Create a full picture through a complicated algorithm that includes both temporal and spatial interpolation.");
		break;
	case LinearBlend:
		desc = tr("Linear blend deinterlacing filter that deinterlaces the given block by filtering all lines with a (1 2 1) filter.");
		break;
	case Median:
		desc = tr("Median deinterlacing filter that deinterlaces the given block by applying a median filter to every second line.");
		break;
	default:
		return QString();
	}
	const auto caps = Item::list()[method].caps;
	QString info;
	if (caps & DoubleRate)
		info += "\n* " + tr("This method supports double the framerate.");
	if (caps & OpenGL)
		info += "\n* " + tr("This method can be accelerated by OpenGL.");
	if (caps & VaApi)
		info += "\n* " + tr("This method can be accelerated by VA-API. In order to activate this filter, You have to enable it in 'Hardware Acceleration'.");
	if (!info.isEmpty())
		desc += "\n" + info;
	return desc;
}


struct DeintWidget::Data {
	QCheckBox *hwacc = nullptr;
	QCheckBox *doubler = nullptr;
	DeintInfo::Method method;
};

DeintWidget::DeintWidget(const DeintInfo &info, bool hwdec, QWidget *parent)
: QWidget(parent), d(new Data) {
	const int caps = info.capabilities();
	d->method = info.method();
	d->doubler = new QCheckBox(tr("Make the framerate doubled"), this);
	d->hwacc = new QCheckBox(tr("Enable hardware acceleration"), this);
	d->doubler->setEnabled(caps & DeintInfo::DoubleRate);
	d->hwacc->setEnabled(!hwdec && (caps & DeintInfo::Software) && (caps & DeintInfo::Hardware));
	d->hwacc->setChecked(hwdec);
	auto vbox = new QVBoxLayout;
	vbox->setMargin(0);
	vbox->addWidget(d->doubler);
	vbox->addWidget(d->hwacc);
	setLayout(vbox);

	setInfo(info);
}

DeintWidget::~DeintWidget() {
	delete d;
}

void DeintWidget::setInfo(const DeintInfo &info) {
	d->doubler->setChecked(info.isDoubleRate());
	d->hwacc->setChecked(info.isHardware());
}

DeintInfo DeintWidget::info() const {
	int flags = 0;
	flags |= d->doubler->isChecked() ? DeintInfo::DoubleRate : DeintInfo::SingleRate;
	flags |= d->hwacc->isChecked() ? DeintInfo::Hardware : DeintInfo::Software;
	return DeintInfo(d->method, flags);
}

DeintInfo::Method DeintWidget::method() const {
	return d->method;
}

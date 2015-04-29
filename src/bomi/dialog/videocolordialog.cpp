#include "videocolordialog.hpp"
#include "video/videocolor.hpp"
#include "dialog/bbox.hpp"
#include <QToolButton>

struct Line {
    QSlider *slider;
    QSpinBox *spin;
};

struct VideoColorDialog::Data {
    bool setting = false;
    VideoColor::Array<Line> lines;
    Update update;
};

VideoColorDialog::VideoColorDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    _SetWindowTitle(this, tr("Video Color Editor"));

    auto grid = new QGridLayout;
    int offset = 0;
    VideoColor::for_type([&] (VideoColor::Type type) {
        if (type == VideoColor::Red) {
            auto hl = new QFrame;
            hl->setFixedHeight(3);
            hl->setFrameShape(QFrame::HLine);
            hl->setFrameShadow(QFrame::Sunken);
            grid->addWidget(hl, type, 0, 1, -1);
            offset = 1;
        }

        auto &l = d->lines[type];
        l.slider = new QSlider(Qt::Horizontal);
        l.spin = new QSpinBox;
        auto button = new QPushButton;
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setFlat(true);
        button->setToolTip(tr("Reset"));
        QPixmap px; px.load(":/img/"_a % VideoColor::name(type) % ".png"_a);
        button->setIcon(px);
        grid->addWidget(button, type + offset, 0);
        grid->addWidget(l.slider, type + offset, 1);
        grid->addWidget(l.spin, type + offset, 2);
        l.slider->setRange(-100, 100);
        l.spin->setRange(-100, 100);
        l.spin->setSuffix(qApp->translate("PrefDialog", " %"));
        connect(l.slider, &QSlider::valueChanged, l.spin, &QSpinBox::setValue);
        connect(SIGNAL_VT(l.spin, valueChanged, int), l.slider, [=] (int v) {
            l.slider->setValue(v);
            if (!d->setting && d->update)
                d->update(color());
        });
        connect(button, &QPushButton::clicked, this,
                [=] () { d->lines[type].spin->setValue(0); });
    });

    auto bbox = new BBox;
    connect(bbox->addButton(BBox::Reset), &QPushButton::clicked,
            this, [=] () { setColor(VideoColor()); });
    const auto close = bbox->addButton(BBox::Close);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    close->setAutoDefault(true);
    close->setDefault(true);
    grid->addWidget(bbox, 7 + 1, 0, 1, -1);

    setLayout(grid);
    setMinimumWidth(400);
    adjustSize();
}

VideoColorDialog::~VideoColorDialog()
{
    delete d;
}

auto VideoColorDialog::setColor(const VideoColor &eq) -> void
{
    d->setting = true;
    VideoColor::for_type([&] (auto type) { d->lines[type].spin->setValue(eq[type]); });
    d->setting = false;
    if (d->update)
        d->update(eq);
}

auto VideoColorDialog::color() const -> VideoColor
{
    VideoColor eq;
    eq.for_type([&] (VideoColor::Type type) {
        eq.set(type, d->lines[type].spin->value());
    });
    return eq;
}

auto VideoColorDialog::setUpdateFunc(Update &&func) -> void
{
    d->update = std::move(func);
}

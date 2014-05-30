#include "openmediabehaviorgroupbox.hpp"
#include "enumcombobox.hpp"
#include "player/openmediainfo.hpp"

struct OpenMediaBehaviorGroupBox::Data {
    QCheckBox *start;
    EnumComboBox<OpenMediaBehavior> *playlist;
};

OpenMediaBehaviorGroupBox::OpenMediaBehaviorGroupBox(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    auto layout = new QVBoxLayout(this);
    d->start = new QCheckBox(tr("Start the playback"), this);
    d->playlist = new EnumComboBox<OpenMediaBehavior>(this);
    layout->addWidget(d->start);
    layout->addWidget(d->playlist);
    auto vbox = static_cast<QVBoxLayout*>(parent->layout());
    vbox->insertWidget(vbox->count()-1, this);
}

OpenMediaBehaviorGroupBox::~OpenMediaBehaviorGroupBox()
{
    delete d;
}

auto OpenMediaBehaviorGroupBox::setValue(const OpenMediaInfo &open) -> void
{
    d->start->setChecked(open.start_playback);
    d->playlist->setCurrentValue(open.behavior);
}
auto OpenMediaBehaviorGroupBox::value() const -> OpenMediaInfo
{
    return OpenMediaInfo(d->start->isChecked(), d->playlist->currentValue());
}

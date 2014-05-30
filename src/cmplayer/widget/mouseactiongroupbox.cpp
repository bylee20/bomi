#include "mouseactiongroupbox.hpp"
#include "misc/keymodifieractionmap.hpp"

struct MouseActionGroupBox::Data {
    QList<QComboBox*> combos;
    QList<KeyModifier> mods;
    QList<QCheckBox*> checks;
};

MouseActionGroupBox::MouseActionGroupBox(QWidget *parent)
    : QGroupBox(parent)
    , d(new Data)
{

}

MouseActionGroupBox::~MouseActionGroupBox()
{
    delete d;
}

auto MouseActionGroupBox::set(const QList<Action> &list) -> void
{
    d->mods << KeyModifier::None << KeyModifier::Ctrl
            << KeyModifier::Shift << KeyModifier::Alt;
    QGridLayout *grid = new QGridLayout(this);
    grid->setMargin(0);
    for (int i=0; i<d->mods.size(); ++i) {
        auto combo = new QComboBox(this);
        for (auto &info : list)
            combo->addItem(info.first, info.second);
        QCheckBox *check = new QCheckBox(this);
        d->combos.append(combo);
        d->checks.append(check);
        if (d->mods[i] != KeyModifier::None) {
            const auto key = QKeySequence((int)d->mods[i]);
            check->setText(key.toString(QKeySequence::NativeText));
        }
        grid->addWidget(check, i, 0, 1, 1);
        grid->addWidget(combo, i, 1, 1, 1);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        combo->setEnabled(check->isChecked());
        connect(check, &QCheckBox::toggled, combo, &QWidget::setEnabled);
    }
}


auto MouseActionGroupBox::setValues(const KeyModifierActionMap &map) -> void {
    for (int i=0; i<d->mods.size(); ++i) {
        const auto info = map[d->mods[i]];
        const int idx = d->combos[i]->findData(info.id);
        if (idx != -1) {
            d->combos[i]->setCurrentIndex(idx);
            d->checks[i]->setChecked(info.enabled);
        }
    }
}

auto MouseActionGroupBox::values() const -> KeyModifierActionMap {
    KeyModifierActionMap map;
    for (int i=0; i<d->mods.size(); ++i) {
        auto &info = map[d->mods[i]];
        info.enabled = d->checks[i]->isChecked();
        info.id = d->combos[i]->currentData().toString();
    }
    return map;
}

#include "shortcutdialog.hpp"
#include "bbox.hpp"

static constexpr int MaxKeyCount = 1;

struct ShortcutDialog::Data {
    QLineEdit *edit = nullptr;
    BBox *bbox;
    QPushButton *begin = nullptr;
    int curIdx = 0;
    int codes[MaxKeyCount];
    auto erase() -> void
    {
        curIdx = 0;
        for (int i=0; i<MaxKeyCount; ++i)
            codes[i] = 0;
        edit->clear();
    }
    auto getShortcut(QKeyEvent *event) -> void
    {
        if (_InRange0(curIdx, MaxKeyCount)) {
            codes[curIdx] = event->key();
            int modifiers = 0;
            if (event->modifiers() & Qt::CTRL)
                modifiers |= Qt::CTRL;
            if (event->modifiers() & Qt::SHIFT)
                modifiers |= Qt::SHIFT;
            if (event->modifiers() & Qt::ALT)
                modifiers |= Qt::ALT;
            if (event->modifiers() & Qt::META)
                modifiers |= Qt::META;
            if (modifiers)
                codes[curIdx] += modifiers;
        }
    }
};

ShortcutDialog::ShortcutDialog(const QKeySequence &shortcut, QWidget *parent)
: QDialog(parent) {
    d = new Data;
    d->edit = new QLineEdit(this);
    d->edit->setReadOnly(true);
    d->bbox = new BBox(this);
    d->begin = d->bbox->addButton(tr("Get Shortcut"), BBox::ActionRole);
    d->bbox->addButton(tr("Erase"), BBox::ActionRole, [=] () { d->erase(); });
    d->bbox->addButton(BBox::Ok);
    d->bbox->addButton(BBox::Cancel);

    for (auto button : d->bbox->buttons()) {
        auto pb = static_cast<QPushButton*>(button);
        pb->setFocusPolicy(Qt::NoFocus);
        pb->setAutoDefault(false);
        pb->setDefault(false);
    }

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(d->edit);
    vbox->addWidget(d->bbox);

    connect(d->begin, &QPushButton::toggled,
            this, [=] (bool on) { if (on) d->erase(); });
    connect(d->bbox, &BBox::accepted, this, &ShortcutDialog::accept);
    connect(d->bbox, &BBox::rejected, this, &ShortcutDialog::reject);
    d->edit->installEventFilter(this);
    d->begin->setCheckable(true);
    d->begin->toggle();
    setShortcut(shortcut);
}

ShortcutDialog::~ShortcutDialog() {
    delete d;
}

auto ShortcutDialog::shortcut() const -> QKeySequence
{
    return QKeySequence(d->codes[0]);
}

auto ShortcutDialog::setShortcut(const QKeySequence &shortcut) -> void
{
    for (int i=0; i<MaxKeyCount; ++i)
        d->codes[i] = shortcut[i];
    d->edit->setText(shortcut.toString(QKeySequence::NativeText));
}

auto ShortcutDialog::eventFilter(QObject *obj, QEvent *event) -> bool
{
    if (obj == d->edit && d->begin->isChecked()
            && event->type() == QEvent::KeyPress) {
        d->getShortcut(static_cast<QKeyEvent *>(event));
        return true;
    } else
        return QDialog::eventFilter(obj, event);
}

auto ShortcutDialog::keyPressEvent(QKeyEvent *event) -> void
{
    QDialog::keyPressEvent(event);
    if (d->begin->isChecked())
        d->getShortcut(event);
}

auto ShortcutDialog::keyReleaseEvent(QKeyEvent *event) -> void
{
    QDialog::keyReleaseEvent(event);
    if (d->begin->isChecked() && d->codes[d->curIdx]) {
        d->edit->setText(shortcut().toString(QKeySequence::NativeText));
        ++d->curIdx;
    }
}

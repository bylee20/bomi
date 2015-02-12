#include "shortcutdialog.hpp"
#include "bbox.hpp"
#include <QKeyEvent>

struct ShortcutDialog::Data {
    QLineEdit *edit = nullptr;
    BBox *bbox;
    QPushButton *begin = nullptr;

    QTimer updater;

    QKeySequence input;
    QString id;
    QueryFunc query;


    auto setOk(bool ok) -> void { bbox->button(BBox::Ok)->setEnabled(ok); }
    auto erase() -> void
    {
        input = QKeySequence();
        edit->clear();
        setOk(true);
    }
    auto getShortcut(QKeyEvent *event) -> void
    {
        if (!begin->isChecked())
            return;
        switch (event->key()) {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
            return;
        }
        int code = event->key();
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
            code += modifiers;
        input = QKeySequence(code);
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

auto ShortcutDialog::setQueryFunction(const QueryFunc &func,
                                      const QString &id) -> void
{
    d->id = id;
    d->query = func;
}

auto ShortcutDialog::shortcut() const -> QKeySequence
{
    return d->input;
}

auto ShortcutDialog::setShortcut(const QKeySequence &shortcut) -> void
{
    d->input = shortcut;
    d->edit->setText(d->input.toString(QKeySequence::NativeText));
}

auto ShortcutDialog::eventFilter(QObject *obj, QEvent *event) -> bool
{
    if (obj == d->edit && event->type() == QEvent::KeyPress) {
        d->getShortcut(static_cast<QKeyEvent *>(event));
        return true;
    } else
        return QDialog::eventFilter(obj, event);
}

auto ShortcutDialog::keyPressEvent(QKeyEvent *event) -> void
{
    QDialog::keyPressEvent(event);
    d->getShortcut(event);
}

auto ShortcutDialog::keyReleaseEvent(QKeyEvent *event) -> void
{
    QDialog::keyReleaseEvent(event);
    if (d->begin->isChecked() && !d->input.isEmpty()) {
        const auto desc = d->query ? d->query(d->id, d->input) : QString();
        const auto key = d->input.toString(QKeySequence::NativeText);
        d->setOk(desc.isEmpty());
        if (desc.isEmpty()) {
            d->edit->setStyleSheet(u"color: black"_q);
            d->edit->setText(key);
        } else {
            d->edit->setText(tr("'%1' is already bound to '%2'").arg(key, desc));
            d->edit->setStyleSheet(u"color: red"_q);
            d->input = QKeySequence();
        }
    }
}

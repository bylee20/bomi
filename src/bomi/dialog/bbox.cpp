#include "bbox.hpp"

BBox::BBox(QWidget *parent)
    : QDialogButtonBox(parent)
{
    m_layout = buttonLayout(this);
}

auto BBox::setStandardButtons(StandardButtons buttons) -> void
{
    uint flags = buttons;
    for (int i=0; i<32 && flags; ++i) {
        const Button button = Button(1 << i);
        if (flags & button) {
            addButton(button);
            flags &= ~button;
        }
    }
}

auto BBox::addButton(StandardButton button) -> QPushButton*
{
    const auto text = buttonText(button, m_layout);
    auto b = QDialogButtonBox::addButton(button);
    b->setText(text);
    return b;
}

auto BBox::buttonLayout(QWidget *w) -> Layout {
    const auto style = w->style();
    const auto layout = style->styleHint(QStyle::SH_DialogButtonLayout, 0, w);
    return static_cast<Layout>(layout);
}

auto BBox::buttonText(Button button) -> QString
{
    const auto layout = qApp->style()->styleHint(QStyle::SH_DialogButtonLayout);
    return buttonText(button, static_cast<Layout>(layout));
}

auto BBox::buttonText(Button button, Layout layout) -> QString
{
    const auto gnome = (layout == GnomeLayout);
    const auto skin = (layout == SkinLayout);
    switch (button) {
    case QDialogButtonBox::Ok:
        return gnome && !skin ? tr("&OK") : tr("OK");
    case QDialogButtonBox::Save:
        return gnome && !skin ? tr("&Save") : tr("Save");
    case QDialogButtonBox::Open:
        return tr("Open");
    case QDialogButtonBox::Cancel:
        return gnome && !skin ? tr("&Cancel") : tr("Cancel");
    case QDialogButtonBox::Close:
        return gnome && !skin ? tr("&Close") : tr("Close");
    case QDialogButtonBox::Apply:
        return tr("Apply");
    case QDialogButtonBox::Reset:
        return tr("Reset");
    case QDialogButtonBox::Help:
        return tr("Help");
    case QDialogButtonBox::Discard:
        if (layout == MacLayout)
            return tr("Don't Save");
        if (layout == GnomeLayout)
            return tr("Close without Saving");
        return tr("Discard");
    case QDialogButtonBox::Yes:
        return skin ? tr("Yes") : tr("&Yes");
    case QDialogButtonBox::YesToAll:
        return skin ? tr("Yes to All") : tr("Yes to &All");
    case QDialogButtonBox::No:
        return skin ? tr("No") : tr("&No");
    case QDialogButtonBox::NoToAll:
        return skin ? tr("No to All") : tr("N&o to All");
    case QDialogButtonBox::SaveAll:
        return tr("Save All");
    case QDialogButtonBox::Abort:
        return tr("Abort");
    case QDialogButtonBox::Retry:
        return tr("Retry");
    case QDialogButtonBox::Ignore:
        return tr("Ignore");
    case QDialogButtonBox::RestoreDefaults:
        return tr("Restore Defaults");
    case QDialogButtonBox::NoButton:
        return QString();
    }
    Q_ASSERT(false);
    return QString();
}

auto BBox::make(QDialog *dlg) -> BBox*
{
    auto bbox = new BBox(dlg);
    bbox->setOrientation(Qt::Horizontal);
    bbox->addButton(Ok);
    bbox->addButton(Cancel);
    connect(bbox, &BBox::accepted, dlg, &QDialog::accept);
    connect(bbox, &BBox::rejected, dlg, &QDialog::reject);
    return bbox;
}

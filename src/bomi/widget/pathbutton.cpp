#include "pathbutton.hpp"
#include <QMetaProperty>

struct PathButton::Data {
    PathButton::Mode mode = PathButton::Folder;
    PathButton::Text text = PathButton::Browse;
    QObject *editor = nullptr;
    QMetaProperty prop;
    QString filter, key;

    auto getText() const -> QString
    {
        switch (text) {
        case Open:
            return tr("Open...");
        case Load:
            return tr("Load...");
        case Browse:
            return tr("Browse...");
        default:
            return QString();
        }
    }
};

PathButton::PathButton(QWidget *parent)
    : QPushButton(parent), d(new Data)
{
    QPushButton::setText(d->getText());

    connect(this, &QPushButton::clicked, this, [=] () {
        switch (d->mode) {
        case Folder: {
            const auto ret = getFolder();
            if (!ret.isEmpty()) {
                emit folderSelected(ret);
                if (d->editor)
                    d->prop.write(d->editor, ret);
            }
            break;
        } case SingleFile: {
            const auto ret = getFile();
            if (!ret.isEmpty()) {
                emit fileSelected(ret);
                if (d->editor)
                    d->prop.write(d->editor, ret);
            }
            break;
        } case MultiFile: {
            const auto ret = getFiles();
            if (!ret.isEmpty()) {
                emit filesSelected(ret);
                if (d->editor)
                    d->prop.write(d->editor, ret);
            }
        } default:
            break;
        }
    });
}

PathButton::~PathButton()
{
    delete d;
}

auto PathButton::set(Mode mode, Text text) -> void
{
    setMode(mode);
    setText(text);
}

auto PathButton::set(Mode mode, QLineEdit *edit) -> void
{
    set(mode, mode == Folder ? Browse : Open, edit);
}

auto PathButton::set(Mode mode, Text text, QLineEdit *edit) -> void
{
    setMode(mode);
    setText(text);
    setEditor(edit);
}

auto PathButton::mode() const -> Mode
{
    return d->mode;
}

auto PathButton::setMode(Mode mode) -> void
{
    d->mode = mode;
}

auto PathButton::setEditor(QLineEdit *edit) -> void
{
    setEditor(edit, "text");
}

auto PathButton::setEditor(QComboBox *cb) -> void
{
    setEditor(cb, "currentText");
}

auto PathButton::setEditor(QObject *obj, const char *p) -> void
{
    const auto mo = obj->metaObject();
    const int idx = mo->indexOfProperty(p);
    if (idx < 0)
        return;
    d->editor = obj;
    d->prop = mo->property(idx);
}

auto PathButton::text() const -> Text
{
    return d->text;
}

auto PathButton::setText(Text text) -> void
{
    if (_Change(d->text, text))
        QPushButton::setText(d->getText());
}

auto PathButton::setIcon(const QIcon &icon) -> void
{
    QPushButton::setIcon(icon);
    if (!icon.isNull())
        setText(Empty);
}

auto PathButton::setFilter(const QString &filter) -> void
{
    d->filter = filter;
}

auto PathButton::setFilter(ExtTypes exts) -> void
{
    d->filter = _ToFilter(exts);
}

auto PathButton::key() const -> QString
{
    return d->key;
}

auto PathButton::setKey(const QString &key) -> void
{
    d->key = key;
}

auto PathButton::getFolder() -> QString
{
    return _GetOpenDir(this, tr("Open Folder"), key());
}

auto PathButton::getFile() -> QString
{
    return _GetOpenFile(this, tr("Open File"), d->filter, key());
}

auto PathButton::getFiles() -> QStringList
{
    return _GetOpenFiles(this, tr("Open Files"), d->filter, key());
}

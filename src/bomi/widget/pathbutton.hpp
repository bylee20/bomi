#ifndef PATHBUTTON_HPP
#define PATHBUTTON_HPP

class PathButton : public QPushButton {
    Q_OBJECT
public:
    enum Mode { Folder, SingleFile, MultiFile };
    enum Text { Empty, Browse, Open, Load };
    PathButton(QWidget *parent = nullptr);
    ~PathButton();
    auto set(Mode mode, QLineEdit *edit) -> void;
    auto set(Mode mode, Text text, QLineEdit *edit) -> void;
    auto set(Mode mode, Text text) -> void;
    auto setMode(Mode mode) -> void;
    auto mode() const -> Mode;
    auto setEditor(QLineEdit *edit) -> void;
    auto setEditor(QComboBox *cb) -> void;
    auto setEditor(QObject *obj, const char *prop) -> void;
    auto setText(Text text) -> void;
    auto text() const -> Text;
    auto setFilter(const QString &filter) -> void;
    auto setFilter(ExtTypes exts) -> void;
    auto setIcon(const QIcon &icon) -> void;
    auto key() const -> QString;
    auto setKey(const QString &key) -> void;
    auto getFolder() -> QString;
    auto getFile() -> QString;
    auto getFiles() -> QStringList;
signals:
    void folderSelected(const QString &folder);
    void fileSelected(const QString &file);
    void filesSelected(const QStringList &files);
private:
    struct Data;
    Data *d;
};

#endif // PATHBUTTON_HPP

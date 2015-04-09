#ifndef OPENMEDIAFOLDERDIALOG_HPP
#define OPENMEDIAFOLDERDIALOG_HPP

class Playlist;

class OpenMediaFolderDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(OpenMediaFolderDialog)
public:
    OpenMediaFolderDialog(QWidget *parent = nullptr,
                          const QString &key = QString());
    ~OpenMediaFolderDialog();
    auto playlist() const -> Playlist;
    auto exec() -> int final;
    auto setFolder(const QString &folder) -> void;
private:
    struct Data;
    Data *d;
};

#endif // OPENMEDIAFOLDERDIALOG_HPP

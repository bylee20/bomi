#ifndef OPENMEDIAFOLDERDIALOG_HPP
#define OPENMEDIAFOLDERDIALOG_HPP

#include "stdafx.hpp"

class Playlist;

class OpenMediaFolderDialog : public QDialog {
    Q_OBJECT
public:
    OpenMediaFolderDialog(QWidget *parent = nullptr);
    ~OpenMediaFolderDialog();
    auto playlist() const -> Playlist;
    auto exec() -> int final;
    auto setCheckedTypes(const QString &types) -> void;
    auto checkedTypes() const -> QString;
private:
    auto updateList() -> void;
    auto checkList(bool checked) -> void;
    auto getFolder() -> void;
    auto updateOpenButton() -> void;
    struct Data;
    Data *d;
};

#endif // OPENMEDIAFOLDERDIALOG_HPP

#ifndef OPENMEDIAFOLDERDIALOG_HPP
#define OPENMEDIAFOLDERDIALOG_HPP

#include "stdafx.hpp"

class Playlist;

class OpenMediaFolderDialog : public QDialog {
    Q_OBJECT
public:
    OpenMediaFolderDialog(QWidget *parent = nullptr);
    ~OpenMediaFolderDialog();
    Playlist playlist() const;
public slots:
    void setFolder(const QString &folder);
    int exec();
signals:

private slots:
    void updateList();
    void checkList(bool checked);
    void getFolder();
    void updateOpenButton();
private:
    struct Data;
    Data *d;
};

#endif // OPENMEDIAFOLDERDIALOG_HPP

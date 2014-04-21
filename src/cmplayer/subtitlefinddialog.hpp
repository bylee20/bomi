#ifndef SUBTITLEFINDDIALOG_HPP
#define SUBTITLEFINDDIALOG_HPP

#include <QDialog>

class Mrl;

class SubtitleFindDialog : public QDialog {
    Q_OBJECT
public:
    SubtitleFindDialog(QWidget *parent = nullptr);
    ~SubtitleFindDialog();
    void find(const Mrl &mrl);
signals:
    void loadRequested(const QString &fileName);
private:
    struct Data;
    Data *d;
};

#endif // SUBTITLEFINDDIALOG_HPP

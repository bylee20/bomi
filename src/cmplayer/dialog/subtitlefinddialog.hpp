#ifndef SUBTITLEFINDDIALOG_HPP
#define SUBTITLEFINDDIALOG_HPP

class Mrl;

class SubtitleFindDialog : public QDialog {
    Q_OBJECT
public:
    SubtitleFindDialog(QWidget *parent = nullptr);
    ~SubtitleFindDialog();
    auto find(const Mrl &mrl) -> void;
signals:
    void loadRequested(const QString &fileName);
private:
    struct Data;
    Data *d;
};

#endif // SUBTITLEFINDDIALOG_HPP

#ifndef SUBTITLEFINDDIALOG_HPP
#define SUBTITLEFINDDIALOG_HPP

class Mrl;

class SubtitleFindDialog : public QDialog {
    Q_OBJECT
public:
    SubtitleFindDialog(const bool save, QWidget *parent = nullptr);
    ~SubtitleFindDialog();
    auto find(const Mrl &mrl) -> void;
    auto setSelectedLangCode(const QString &langCode) -> void;
    auto selectedLangCode() const -> QString;
signals:
    void loadRequested(const QString &fileName);
private:
    struct Data;
    Data *d;
};

#endif // SUBTITLEFINDDIALOG_HPP

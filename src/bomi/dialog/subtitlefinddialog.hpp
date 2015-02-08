#ifndef SUBTITLEFINDDIALOG_HPP
#define SUBTITLEFINDDIALOG_HPP

class Mrl;

class SubtitleFindDialog : public QDialog {
    Q_OBJECT
public:
    SubtitleFindDialog(QWidget *parent = nullptr);
    ~SubtitleFindDialog();
    auto setOptions(bool preserve, const QString &format, const QString &fb) -> void;
    auto find(const Mrl &mrl) -> void;
    auto setSelectedLangCode(const QString &langCode) -> void;
    auto selectedLangCode() const -> QString;
    static auto defaultFileNameFormat() -> QString;
signals:
    void loadRequested(const QString &fileName);
private:
    struct Data;
    Data *d;
};

#endif // SUBTITLEFINDDIALOG_HPP

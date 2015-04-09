#ifndef SUBTITLEFINDDIALOG_HPP
#define SUBTITLEFINDDIALOG_HPP

class Mrl;

class SubtitleFindDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(SubtitleFindDialog)
    using Load = std::function<void(const QString&)>;
public:
    SubtitleFindDialog(QWidget *parent = nullptr);
    ~SubtitleFindDialog();
    auto setOptions(bool preserve, const QString &format, const QString &fb) -> void;
    auto find(const Mrl &mrl) -> void;
    auto setLoadFunc(Load &&load) -> void;
    static auto defaultFileNameFormat() -> QString;
private:
    struct Data;
    Data *d;
};

#endif // SUBTITLEFINDDIALOG_HPP

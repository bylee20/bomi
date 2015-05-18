#ifndef URLDIALOG_HPP
#define URLDIALOG_HPP

class Playlist;                         class EncodingInfo;

class UrlDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(UrlDialog)
public:
    UrlDialog(QWidget *parent = 0, const QString &key = QString());
    ~UrlDialog();
    auto setUrl(const QUrl &url) -> void;
    auto url() const -> QUrl;
    auto isPlaylist() const -> bool;
    auto encoding() const -> EncodingInfo;
private:
    auto showEvent(QShowEvent *event) -> void final;
    auto accept() -> void final;
    struct Data;
    Data *d;
};
#endif // URLDIALOG_HPP

#ifndef YOUTUBEDIALOG_HPP
#define YOUTUBEDIALOG_HPP

class YouTubeDialog : public QProgressDialog {
    Q_OBJECT
public:
    YouTubeDialog(QWidget *parent = nullptr);
    ~YouTubeDialog();
    auto userAgent() const -> QString;
    auto setUserAgent(const QString &ua) -> void;
    auto setProgram(const QString &program) -> void;
    auto program() const -> QString;
    auto setTimeout(int timeout) -> void;
    auto timeout() const -> int;
    auto cookies() const -> QString;
    auto translate(const QUrl &url) -> void;
    auto videoUrl() const -> QString;
    auto exec() -> int override;
private:
    struct Data;
    Data *d;
};

#endif // YOUTUBEDIALOG_HPP

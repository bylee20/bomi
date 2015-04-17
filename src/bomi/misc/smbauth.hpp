#ifndef SMBAUTH_HPP
#define SMBAUTH_HPP

class SmbDir;                           class Mrl;

class SmbAuth {
public:
    enum Error {
        NoError,
        Unsupported,
        InvalidUrl,
        OutOfMemory,
        NoSmbConf,
        InvalidParam,
        NotFile,
        NoPermission,
        NotExistingShare,
        NotExistingPath,
        AlreadyExists,
        UnknownError
    };
    auto setUsername(const QString &user) { m_username = user; }
    auto setPassword(const QString &pass) { m_password = pass; }
    auto password() const -> QString { return m_password; }
    auto username() const -> QString { return m_username; }
    auto process(const QUrl &url) -> Error;
    auto translate(const QUrl &url) -> QUrl;
    auto lastError() const -> Error { return m_lastError; }
    auto getNewAuthInfo() -> bool;
    auto setGetAuthInfo(std::function<bool(SmbAuth*)> &&func)
        { m_getAuthInfo = func; }
    auto lastErrorString() const -> QString { return errorString(m_lastError); }
    static auto errorString(Error error) -> QString;
    auto openDir(const Mrl &mrl) -> QSharedPointer<SmbDir>;
private:
    QString m_username, m_password;
    std::function<bool(SmbAuth*)> m_getAuthInfo;
    Error m_lastError = NoError;
};

class SmbAuthDialog : public QDialog {
    Q_OBJECT
public:
    SmbAuthDialog(QWidget *parent = nullptr);
    ~SmbAuthDialog();
    auto setInformativeText(const QString &text) -> void;
    auto setAuthInfo(const SmbAuth &smb) -> void;
    auto authInfo() const -> SmbAuth;
private:
    struct Data;
    Data *d;
};

#endif // SMBAUTH_HPP

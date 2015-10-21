#include "smbauth.hpp"
#include "player/mrl.hpp"
#include "dialog/bbox.hpp"
#include "configure.hpp"
#if HAVE_SAMBA
#include <libsmbclient.h>
#endif
#include <errno.h>

SIA smb_auth_fn(const char */*server*/, const char */*share*/,
                char *workgroup, int wgmaxlen, char */*username*/, int /*unmaxlen*/,
                char */*password*/, int /*pwmaxlen*/) -> void
{
    strncpy(workgroup, "LAN", wgmaxlen - 1);
}

auto SmbAuth::translate(const QUrl &input) -> QUrl
{
    QUrl url = input;
    if (url.userName().isEmpty()) {
        url.setUserName(m_username);
        url.setPassword(m_password);
    } else if (url.userName() == m_username) {
        if (url.password().isEmpty())
            url.setPassword(m_password);
        else
            m_password = url.password();
    } else {
        m_username = url.userName();
        m_password = url.password();
    }
    Q_ASSERT(m_username == url.userName());
    Q_ASSERT(m_password == url.password());
    return url;
}

SIA errorForInit(int e) -> SmbAuth::Error
{
    switch (e) {
    case ENOMEM: return SmbAuth::OutOfMemory;
    case ENOENT: return SmbAuth::NoSmbConf;
    default:     return SmbAuth::UnknownError;
    }
}

class SmbDir {
public:
    SmbDir(const QUrl &url)
    {
        Q_UNUSED(url);
        m_error = SmbAuth::Unsupported;
#if HAVE_SAMBA
        m_url = url;
        m_dh = smbc_opendir(url.toString(QUrl::FullyEncoded).toLatin1());
        if (m_dh < 0) {
            switch (errno) {
            case EACCES:
                m_error = SmbAuth::NoPermission;
                break;
            case EINVAL:
                m_error = SmbAuth::InvalidUrl;
                break;
            case ENOENT:
                m_error = SmbAuth::NotExistingPath;
                break;
            case ENOMEM:
                m_error = SmbAuth::OutOfMemory;
                break;
            case EPERM:
            case ENODEV:
                m_error = SmbAuth::NotExistingShare;
                break;
            default: break;
            }
            return;
        }
        smbc_dirent *child = nullptr;
        while ((child = smbc_readdir(m_dh))) {
            qDebug() << QByteArray(child->name, child->namelen);
        }
#endif
    }
    ~SmbDir()
    {
#if HAVE_SAMBA
        if (m_dh >= 0)
            smbc_closedir(m_dh);
#endif
    }
    auto lastError() const -> SmbAuth::Error { return m_error; }
private:
    QUrl m_url;
    Q_DISABLE_COPY(SmbDir)
    int m_dh = -1;
    SmbAuth::Error m_error = SmbAuth::NoError;
};

auto SmbAuth::openDir(const Mrl &mrl) -> QSharedPointer<SmbDir>
{
    Q_UNUSED(mrl);
#if HAVE_SAMBA
    const int err = smbc_init(smb_auth_fn, 1);
    if (err < 0)
        return QSharedPointer<SmbDir>();
    const auto str = mrl.toString();
    const int idx = str.lastIndexOf('/'_q);
    if (idx < 0)
        return QSharedPointer<SmbDir>();
    return QSharedPointer<SmbDir>(new SmbDir(QUrl(str.left(idx))));
#else
    return QSharedPointer<SmbDir>();
#endif
}

auto SmbAuth::process(const QUrl &url) -> Error
{
#if HAVE_SAMBA
    const int err = smbc_init(smb_auth_fn, 1);
    if (err < 0)
        return m_lastError = errorForInit(errno);

    const int fd = smbc_open(url.toString(QUrl::FullyEncoded).toLatin1(), O_RDONLY, 0644);
    if (fd < 0)
        return m_lastError = [&] () {
            switch (errno) {
            case ENOMEM: return OutOfMemory;
            case EINVAL: return InvalidParam;
            case EEXIST: return AlreadyExists;
            case EISDIR: return NotFile;
            case EACCES: return NoPermission;
            case ENODEV: return NotExistingShare;
            case ENOENT: return NotExistingPath;
            default:     return UnknownError;
            }
        }();

    smbc_close(fd);
    return m_lastError = NoError;
#else
    Q_UNUSED(url);
    return m_lastError = Unsupported;
#endif
}

auto SmbAuth::getNewAuthInfo() -> bool
{
    if (!m_getAuthInfo)
        return false;
    SmbAuth auth = *this;
    if (!m_getAuthInfo(&auth))
        return false;
    setUsername(auth.username());
    setPassword(auth.password());
    return true;
}

auto SmbAuth::errorString(Error error) -> QString
{
    switch (error) {
    case NoError:
        return u"No error"_q;
    case Unsupported:
        return u"smb:// is not supported"_q;
    case InvalidUrl:
        return u"Invalid URL"_q;
    case OutOfMemory:
        return u"Out of memory"_q;
    case NoSmbConf:
        return u"No smb.conf found"_q;
    case InvalidParam:
        return u"Invalid parameters"_q;
    case NotFile:
        return u"URL is not a file"_q;
    case NoPermission:
        return u"No permission to access"_q;
    case NotExistingShare:
        return u"Given sharing does not exist"_q;
    case NotExistingPath:
        return u"Given path does not exist in server"_q;
    case AlreadyExists:
        return u"Given file already exists"_q;
    case UnknownError:
        return u"Unknown error"_q;
    }
    return QString();
}

/******************************************************************************/

struct SmbAuthDialog::Data {
    QLineEdit *username, *password;
    QLabel *text = nullptr;
};

SmbAuthDialog::SmbAuthDialog(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    _SetWindowTitle(this, tr("Authentication Information for SMB"));
    _New(d->username);
    _New(d->password);
    _New(d->text);

    d->text->setWordWrap(true);
    d->password->setEchoMode(QLineEdit::Password);

    auto form = new QFormLayout;
    form->addWidget(d->text);
    form->addRow(tr("Username"), d->username);
    form->addRow(tr("Password"), d->password);
    form->addRow(BBox::make(this));
    setLayout(form);
}

SmbAuthDialog::~SmbAuthDialog()
{
    delete d;
}

auto SmbAuthDialog::setAuthInfo(const SmbAuth &smb) -> void
{
    switch (smb.lastError()) {
    case SmbAuth::NoPermission:
        d->text->setText(tr("No permission granted.\nPlease confirm authentication information and try again."));
        break;
    default:
        d->text->setText(tr("Please input information for authentication."));
        break;
    }

    d->username->setText(smb.username());
    d->password->setText(smb.password());

    adjustSize();
}

auto SmbAuthDialog::authInfo() const -> SmbAuth
{
    SmbAuth smb;
    smb.setUsername(d->username->text());
    smb.setPassword(d->password->text());
    return smb;
}

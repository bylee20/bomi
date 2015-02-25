#include "smbauth.hpp"
#include "dialog/bbox.hpp"
#include <libsmbclient.h>
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
    if (url.userName() == m_username) {
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

auto SmbAuth::process(const QUrl &url) -> Error
{
    const int err = smbc_init(smb_auth_fn, 1);
    if (err < 0)
        return m_lastError = [&] () {
            switch (errno) {
            case ENOMEM: return OutOfMemory;
            case ENOENT: return NoSmbConf;
            default:     return UnknownError;
            }
        }();

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
        return u"Unkown error"_q;
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
    _SetWindowTitle(this, tr("Authentication Informations for SMB"));
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
        d->text->setText(tr("No permission granted.\nPlease confirm authentication informations and try again."));
        break;
    default:
        d->text->setText(tr("Please input informations for authentication."));
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

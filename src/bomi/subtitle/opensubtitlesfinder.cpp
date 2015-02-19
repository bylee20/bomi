#include "opensubtitlesfinder.hpp"
#include "player/mrl.hpp"
#include "misc/xmlrpcclient.hpp"
#include "misc/locale.hpp"

SIA _Args() -> QVariantList { return QVariantList(); }
auto translator_display_language(const QString &iso) -> QString;

struct OpenSubtitlesFinder::Data {
    State state = Unavailable;
    OpenSubtitlesFinder *p = nullptr;
    XmlRpcClient client;
    QString token, error;
    void setState(State s) {
        if (_Change(state, s)) {
            if (state != Error)
                error.clear();
            emit p->stateChanged();
        }
    }
    void logout() {
        if (!token.isEmpty()) {
            client.call(u"LogOut"_q, QVariantList() << token);
            token.clear();
        }
        setState(Unavailable);
    }
    void login() {
        if (state == Connecting)
            return;
        setState(Connecting);
        const auto args = _Args() << u""_q << u""_q << u""_q
                                  << u"CMPlayerXmlRpcClient v0.1"_q;
        client.call(u"LogIn"_q, args, [this] (const QVariantList &results) {
            if (!results.isEmpty()) {
                const auto map = results[0].toMap();
                token = map[u"token"_q].toString();
                if (token.isEmpty())
                    setError(map[u"status"_q].toString());
                else
                    setState(Available);
            } else
                setError(tr("Cannot connect to server"));
        });
    }
    void setError(const QString &error) {
        this->error = error;
        setState(Error);
    }
};

OpenSubtitlesFinder::OpenSubtitlesFinder(QObject *parent)
: QObject(parent), d(new Data) {
    d->p = this;
    d->client.setUrl(QUrl(u"http://api.opensubtitles.org/xml-rpc"_q));
    d->client.setCompressed(true);
    d->login();
}

OpenSubtitlesFinder::~OpenSubtitlesFinder() {
    d->logout();
    delete d;
}

auto OpenSubtitlesFinder::find(const Mrl &mrl) -> bool
{
    if (d->state != Available)
        return false;
    auto fileName = mrl.toLocalFile();
    if (fileName.isEmpty())
        return false;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return false;
    const auto bytes = file.size();
    constexpr int len = 64*1024;
    if (bytes < len)
        return false;
    d->setState(Finding);
    constexpr int chunks = len/sizeof(quint64);
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    auto sum = [&chunks, &file, &in] (qint64 offset) {
        file.seek(offset); quint64 hash = 0, chunk = 0;
        for (int i=0; i<chunks; ++i) { in >> chunk; hash += chunk; } return hash;
    };
    quint64 h = bytes + sum(0) + sum(bytes-len);
    auto hash = QString::number(h, 16);
    if (hash.size() < 16)
        hash = _N(0, 10, 16-hash.size(), '0'_q) + hash;

    QVariantMap map;
    map[u"sublanguageid"_q] = u"all"_q;
    map[u"moviehash"_q] = hash;
    map[u"moviebytesize"_q] = bytes;
    const auto args = _Args() << d->token << QVariant(QVariantList() << map);
    d->client.call(u"SearchSubtitles"_q, args,
                   [this] (const QVariantList &results) {
        d->setState(Available);
        if (results.isEmpty() || results.first().type() != QVariant::Map) {
            emit found(QVector<SubtitleLink>());
        } else {
            const auto list = results.first().toMap()[u"data"_q].toList();
            QVector<SubtitleLink> links;
            for (auto &it : list) {
                if (it.type() != QVariant::Map)
                    continue;
                auto const map = it.toMap();
                SubtitleLink link;
                link.fileName = map[u"SubFileName"_q].toString();
                link.date = map[u"SubAddDate"_q].toString();
                link.url = map[u"SubDownloadLink"_q].toString();
                link.langCode = map[u"SubLanguageID"_q].toString();
                if (link.langCode.isEmpty())
                    link.langCode = map[u"ISO639"_q].toString();
                if (link.langCode.isEmpty())
                    link.langCode = map[u"LanguageName"_q].toString();
                links.append(link);
            }
            emit found(links);
        }
    });
    return true;
}

auto OpenSubtitlesFinder::state() const -> OpenSubtitlesFinder::State
{
    return d->state;
}

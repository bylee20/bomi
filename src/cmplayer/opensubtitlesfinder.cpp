#include "opensubtitlesfinder.hpp"
#include "translator.hpp"
#include "xmlrpcclient.hpp"
#include "mrl.hpp"

static inline QVariantList _Args() { return QVariantList(); }

struct OpenSubtitlesFinder::Data {
	State state = Unavailable;
	OpenSubtitlesFinder *p = nullptr;
	XmlRpcClient client;
	QString token, error;
	void setState(State s) { if (_Change(state, s)) emit p->stateChanged(); }
	void logout() {
		if (!token.isEmpty()) {
			client.call("LogOut", QVariantList() << token);
			token.clear();
		}
		setState(Unavailable);
	}
	void login() {
		if (state == Connecting)
			return;
		setState(Connecting);
		const auto args = _Args() << "" << "" << "" << "OS Test User Agent v1";
		client.call("LogIn", args, [this] (const QVariantList &results) {
			if (!results.isEmpty()) {
				const auto map = results[0].toMap();
				token = map[_L("token")].toString();
				if (token.isEmpty())
					setError(map[_L("status")].toString());
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
	d->client.setUrl(QUrl("http://api.opensubtitles.org/xml-rpc"));
	d->client.setCompressed(true);
	d->login();
}

OpenSubtitlesFinder::~OpenSubtitlesFinder() {
	d->logout();
	delete d;
}

bool OpenSubtitlesFinder::find(const Mrl &mrl) {
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
		hash = _N(0, 10, 16-hash.size(), _L('0')) + hash;

	QVariantMap map;
	map[_L("sublanguageid")] = _L("all");
	map[_L("moviehash")] = hash;
	map[_L("moviebytesize")] = bytes;
	const auto args = _Args() << d->token << QVariant(QVariantList() << map);
	d->client.call("SearchSubtitles", args, [this] (const QVariantList &results) {
		d->setState(Available);
		if (results.isEmpty() || results.first().type() != QVariant::Map) {
			emit found(QList<SubtitleLink>());
		} else {
			const auto list = results.first().toMap()[_L("data")].toList();
			QList<SubtitleLink> links;
			for (auto &it : list) {
				if (it.type() != QVariant::Map)
					continue;
				auto const map = it.toMap();
				SubtitleLink link;
				link.fileName = map[_L("SubFileName")].toString();
				link.date = map[_L("SubAddDate")].toString();
				link.url = map[_L("SubDownloadLink")].toString();
				auto iso = map[_L("SubLanguageID")].toString();
				if (iso.isEmpty())
					iso = map[_L("ISO639")].toString();
				if (iso.isEmpty())
					link.language = map[_L("LanguageName")].toString();
				else
					link.language = Translator::displayLanguage(iso);
				links.append(link);
			}
			emit found(links);
		}
	});
	return true;
}

OpenSubtitlesFinder::State OpenSubtitlesFinder::state() const {
	return d->state;
}

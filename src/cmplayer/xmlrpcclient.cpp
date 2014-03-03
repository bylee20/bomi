#include "xmlrpcclient.hpp"
#include "global.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(XML-RPC)

static QDomNode toValueNode(QDomDocument &doc, const QVariant &var) {
	auto value = doc.createElement(_L("value"));
	QDomElement elem;
	auto el = [&doc] (const char *tag) { return doc.createElement(_L(tag)); };
	auto tn = [&doc] (const QString &text) { return doc.createTextNode(text); };
	auto te = [el, tn] (const char *tag, const QString &text) { auto e = el(tag); e.appendChild(tn(text)); return e; };
	switch (var.type()) {
	case QVariant::String:
		elem = te("string", var.toString());
		break;
	case QVariant::Double:
		elem = te("double", var.toString());
		break;
	case QVariant::Int:
	case QVariant::UInt:
	case QVariant::LongLong:
	case QVariant::ULongLong:
		elem = te("int", var.toString());
		break;
	case QVariant::Bool:
		elem = te("boolean", QString::number(var.toInt()));
		break;
	case QVariant::List: {
		const auto list = var.toList();
		elem = doc.createElement(_L("array"));
		auto data = elem.appendChild(doc.createElement(_L("data")));
		for (auto &it : list)
			data.appendChild(toValueNode(doc, it));
		break;
	} case QVariant::Map: {
		const auto map = var.toMap();
		elem = doc.createElement(_L("struct"));
		for (auto it = map.begin(); it != map.end(); ++it) {
			auto member = elem.appendChild(el("member"));
			member.appendChild(te("name", it.key()));
			member.appendChild(toValueNode(doc, *it));
		}
		break;
	} default:
		_Error("%% was not handle. Convert it to string...", var.typeName());
		elem = te("string", var.toString());
		break;
	}
	value.appendChild(elem);
	return value;
}

struct XmlRpcClient::Data {
	QNetworkAccessManager nam;
	QNetworkRequest request;
	QString lastCall;
	bool compressed = false;
};

XmlRpcClient::XmlRpcClient(QObject *parent)
: QObject(parent), d(new Data) {
	d->request.setHeader(QNetworkRequest::UserAgentHeader, "CMPlayerXmlRpcClient/0.1");
	d->request.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
}

XmlRpcClient::~XmlRpcClient() {
	delete d;
}

void XmlRpcClient::setUrl(const QUrl &url) {
	d->request.setUrl(url);
}

QNetworkReply *XmlRpcClient::postCall(const QString &method, const QList<QVariant> &args) {
	QDomDocument doc;
	doc.appendChild(doc.createProcessingInstruction("xml", R"(version="1.0" encoding="UTF-8")"));

	auto call = doc.createElement("methodCall");
	doc.appendChild(call);

	auto name = doc.createElement("methodName");
	name.appendChild(doc.createTextNode(method));
	call.appendChild(name);

	if (!args.isEmpty()) {
		auto params = doc.createElement("params");
		for (int i=0; i<args.size(); ++i) {
			params.appendChild(doc.createElement(_L("param"))).appendChild(toValueNode(doc, args[i]));
		}
		call.appendChild(params);
	}
	d->lastCall = doc.toString();
	return d->nam.post(d->request, d->lastCall.toUtf8());
}

QVariant parseValue(const QDomElement &elem) {
	Q_ASSERT(elem.tagName() == _L("value"));
	auto type = elem.firstChildElement();
	if (type.isNull())
		return elem.text();
	auto tag = type.tagName();
	if (tag == _L("string")) {
		return type.text();
	} else if (tag == _L("double")) {
		return type.text().toDouble();
	} else if (tag == _L("boolean")) {
		return !!type.text().toInt();
	} else if (tag == _L("array")) {
		QVariantList list;
		auto data = type.firstChildElement(_L("data"));
		auto value = data.firstChildElement(_L("value"));
		while (!value.isNull()) {
			list.append(parseValue(value));
			value = value.nextSiblingElement(_L("value"));
		}
		return list;
	} else if (tag == _L("struct")) {
		QMap<QString, QVariant> map;
		auto member = type.firstChildElement(_L("member"));
		while (!member.isNull()) {
			auto name = member.firstChildElement(_L("name")).text();
			auto value = parseValue(member.firstChildElement(_L("value")));
			map[name] = value;
			member = member.nextSiblingElement(_L("member"));
		}
		return map;
	} else
		_Error("'%%' element was not handle.", tag);
	return QVariant();
}

QVariantList XmlRpcClient::parseResponse(const QByteArray &reply, bool compressed) {
	QDomDocument doc;
	if (compressed)
		doc.setContent(_Uncompress(reply));
	else
		doc.setContent(reply);
	auto elem = doc.firstChildElement(_L("methodResponse"));
	if (elem.isNull())
		return QVariantList();
	elem = elem.firstChildElement(_L("params"));
	if (elem.isNull()) {
		// fault
		return QVariantList();
	}
	QVariantList params;
	auto param = elem.firstChildElement(_L("param"));
	while (!param.isNull()) {
		auto value = param.firstChildElement(_L("value"));
		if (value.isNull())
			return QVariantList();
		params.append(parseValue(value));
		param = param.nextSiblingElement();
	}
	return params;
}

QString XmlRpcClient::lastCall() const {
	return d->lastCall;
}

void XmlRpcClient::setCompressed(bool compressed) {
	if (_Change(d->compressed, compressed))
		d->request.setRawHeader("Accept-Encoding", d->compressed ? "gzip" : "");
}

bool XmlRpcClient::isCompressed() const {
	return d->compressed;
}

#include "xmlrpcclient.hpp"
#include "misc/log.hpp"
#include <QDomNode>

DECLARE_LOG_CONTEXT(XML-RPC)

static QDomNode toValueNode(QDomDocument &doc, const QVariant &var) {
    auto value = doc.createElement(u"value"_q);
    QDomElement elem;
    auto el = [&doc] (const QString &tag) { return doc.createElement(tag); };
    auto tn = [&doc] (const QString &text) { return doc.createTextNode(text); };
    auto te = [el, tn] (const QString &tag, const QString &text)
        { auto e = el(tag); e.appendChild(tn(text)); return e; };
    switch (var.type()) {
    case QVariant::String:
        elem = te(u"string"_q, var.toString());
        break;
    case QVariant::Double:
        elem = te(u"double"_q, var.toString());
        break;
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        elem = te(u"int"_q, var.toString());
        break;
    case QVariant::Bool:
        elem = te(u"boolean"_q, QString::number(var.toInt()));
        break;
    case QVariant::List: {
        const auto list = var.toList();
        elem = doc.createElement(u"array"_q);
        auto data = elem.appendChild(doc.createElement(u"data"_q));
        for (auto &it : list)
            data.appendChild(toValueNode(doc, it));
        break;
    } case QVariant::Map: {
        const auto map = var.toMap();
        elem = doc.createElement(u"struct"_q);
        for (auto it = map.begin(); it != map.end(); ++it) {
            auto member = elem.appendChild(el(u"member"_q));
            member.appendChild(te(u"name"_q, it.key()));
            member.appendChild(toValueNode(doc, *it));
        }
        break;
    } default:
        _Error("%% was not handle. Convert it to string...", var.typeName());
        elem = te(u"string"_q, var.toString());
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
    : QObject(parent)
    , d(new Data)
{
    d->request.setHeader(QNetworkRequest::UserAgentHeader,
                         u"CMPlayerXmlRpcClient/0.1"_q);
    d->request.setHeader(QNetworkRequest::ContentTypeHeader,
                         u"text/xml"_q);
}

XmlRpcClient::~XmlRpcClient()
{
    delete d;
}

auto XmlRpcClient::setUrl(const QUrl &url) -> void
{
    d->request.setUrl(url);
}

auto XmlRpcClient::postCall(const QString &method,
                            const QVariantList &args) -> QNetworkReply*
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction(
                        u"xml"_q, uR"(version="1.0" encoding="UTF-8")"_q
                    ));

    auto call = doc.createElement(u"methodCall"_q);
    doc.appendChild(call);

    auto name = doc.createElement(u"methodName"_q);
    name.appendChild(doc.createTextNode(method));
    call.appendChild(name);

    if (!args.isEmpty()) {
        auto params = doc.createElement(u"params"_q);
        for (int i=0; i<args.size(); ++i)
            params.appendChild(doc.createElement(u"param"_q))
                  .appendChild(toValueNode(doc, args[i]));
        call.appendChild(params);
    }
    d->lastCall = doc.toString();
    return d->nam.post(d->request, d->lastCall.toUtf8());
}

QVariant parseValue(const QDomElement &elem) {
    Q_ASSERT(elem.tagName() == "value"_a);
    auto type = elem.firstChildElement();
    if (type.isNull())
        return elem.text();
    auto tag = type.tagName();
    if (tag == "string"_a) {
        return type.text();
    } else if (tag == "double"_a) {
        return type.text().toDouble();
    } else if (tag == "boolean"_a) {
        return !!type.text().toInt();
    } else if (tag == "array"_a) {
        QVariantList list;
        auto data = type.firstChildElement(u"data"_q);
        auto value = data.firstChildElement(u"value"_q);
        while (!value.isNull()) {
            list.append(parseValue(value));
            value = value.nextSiblingElement(u"value"_q);
        }
        return list;
    } else if (tag == "struct"_a) {
        QMap<QString, QVariant> map;
        auto member = type.firstChildElement(u"member"_q);
        while (!member.isNull()) {
            auto name = member.firstChildElement(u"name"_q).text();
            auto value = parseValue(member.firstChildElement(u"value"_q));
            map[name] = value;
            member = member.nextSiblingElement(u"member"_q);
        }
        return map;
    } else if (tag == "int"_a)
        return type.text().toInt();
    else {
        _Error("'%%' element was not handle.", tag);
    }
    return QVariant();
}

auto XmlRpcClient::parseResponse(const QByteArray &reply,
                                 bool compressed) -> QVariantList
{
    QDomDocument doc;
    if (compressed)
        doc.setContent(_Uncompress(reply));
    else
        doc.setContent(reply);
    auto elem = doc.firstChildElement(u"methodResponse"_q);
    if (elem.isNull())
        return QVariantList();
    elem = elem.firstChildElement(u"params"_q);
    if (elem.isNull()) {
        // fault
        return QVariantList();
    }
    QVariantList params;
    auto param = elem.firstChildElement(u"param"_q);
    while (!param.isNull()) {
        auto value = param.firstChildElement(u"value"_q);
        if (value.isNull())
            return QVariantList();
        params.append(parseValue(value));
        param = param.nextSiblingElement();
    }
    return params;
}

auto XmlRpcClient::lastCall() const -> QString
{
    return d->lastCall;
}

auto XmlRpcClient::setCompressed(bool compressed) -> void
{
    if (_Change(d->compressed, compressed))
        d->request.setRawHeader("Accept-Encoding", d->compressed ? "gzip" : "");
}

auto XmlRpcClient::isCompressed() const -> bool
{
    return d->compressed;
}

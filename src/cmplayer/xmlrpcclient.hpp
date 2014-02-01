#ifndef XMLRPCCLIENT_HPP
#define XMLRPCCLIENT_HPP

#include "stdafx.hpp"

class XmlRpcClient : public QObject {
	Q_OBJECT
public:
	static auto pass(const QVariantList &) -> void {}
	XmlRpcClient(QObject *parent = nullptr);
	~XmlRpcClient();
	void setUrl(const QUrl &url);
	void setCompressed(bool compressed);
	bool isCompressed() const;
	template<typename F = decltype(pass)>
	QMetaObject::Connection call(const QString &method, F onFinished = pass) { return call(method, QVariantList(), onFinished); }
	template<typename F = decltype(pass)>
	QMetaObject::Connection call(const QString &method, const QList<QVariant> &args, F onFinished = pass) {
		auto reply = postCall(method, args);
		const bool comp = isCompressed();
		return connect(reply, &QNetworkReply::finished, [onFinished, reply, comp] () {
			onFinished(parseResponse(reply->readAll(), comp));
			reply->deleteLater();
		});
	}
	QNetworkReply *postCall(const QString &method, const QList<QVariant> &args = QList<QVariant>());
	QString lastCall() const;
private:
	static QVariantList parseResponse(const QByteArray &reply, bool compressed);
	struct Data;
	Data *d;
};

#endif // XMLRPCCLIENT_HPP

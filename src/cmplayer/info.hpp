#ifndef INFO_HPP
#define INFO_HPP

#include <QtCore/QStringList>

class Info {
public:
	class ExtList : public QStringList {
	public:
		ExtList() {}
		ExtList(const QStringList &other): QStringList(other) {}
		ExtList(const ExtList &other): QStringList(other) {}
		QString toFilter() const;
		QStringList toNameFilter() const;
	};
	~Info();
	static const QString &privatePath() {return self.m_privPath;}
	static const ExtList &videoExt() {return self.m_videoExt;}
	static const ExtList &audioExt() {return self.m_audioExt;}
	static const ExtList &subtitleExt() {return self.m_subExt;}
	static const ExtList &playlistExt() {return self.m_plExt;}
	static QStringList videoNameFilter() {return self.m_videoExt.toNameFilter();}
	static QStringList audioNameFilter() {return self.m_audioExt.toNameFilter();}
	static QStringList subtitleNameFilter() {return self.m_subExt.toNameFilter();}
	static QStringList playlistNameFilter() {return self.m_plExt.toNameFilter();}
	static const char *version() {return "0.5.3";}
	static QString mediaExtFilter();
	static QStringList mediaNameFilter() {return videoNameFilter() += audioNameFilter();}
	static const char *pluginPath();
private:
	Info();
	ExtList m_videoExt, m_audioExt, m_subExt, m_plExt;
	QString m_privPath;
	static Info self;
};

#endif // INFO_HPP

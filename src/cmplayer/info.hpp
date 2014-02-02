#ifndef INFO_HPP
#define INFO_HPP

#include "stdafx.hpp"

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
	static const ExtList &readableImageExt();
	static const ExtList &writableImageExt();
	static const ExtList &videoExt() {return self.m_videoExt;}
	static const ExtList &audioExt() {return self.m_audioExt;}
	static const ExtList &subtitleExt() {return self.m_subExt;}
	static const ExtList &playlistExt() {return self.m_plExt;}
	static QStringList videoNameFilter() {return self.m_videoExt.toNameFilter();}
	static QStringList audioNameFilter() {return self.m_audioExt.toNameFilter();}
	static QStringList subtitleNameFilter() {return self.m_subExt.toNameFilter();}
	static QStringList playlistNameFilter() {return self.m_plExt.toNameFilter();}
	static constexpr int versionNumber() { return 0x00808; }
	static constexpr const char *version() {return "0.8.8";}
	static constexpr const char *name() {return "CMPlayer";}
	static QString mediaExtFilter();
	static QStringList mediaNameFilter() {return videoNameFilter() + audioNameFilter() + readableImageExt().toNameFilter();}
	static const char *pluginPath();
private:
	Info();
	ExtList m_videoExt, m_audioExt, m_subExt, m_plExt, m_rImgExt, m_wImgExt;
	QString m_privPath;
	static Info self;
};

#endif // INFO_HPP

#ifndef VLCMEDIA_H
#define VLCMEDIA_H

#include "mrl.hpp"
#include <vlc/vlc.h>
#include <QtCore/QSharedData>

class VLCMedia {
public:
	VLCMedia(const Mrl &mrl);
	VLCMedia(const VLCMedia &rhs);
	VLCMedia &operator = (const VLCMedia &rhs);
	~VLCMedia();
	const libvlc_media_t *media() const;
	libvlc_media_t *media();
	Mrl mrl() const;
	void addOption(const QString &opt);
	void addOption(const QStringList &opt);
	void addOption(const QString &o, const QString &v) {addOption(o + "=" + v);}
	void addOption(const QString &o, int v) {addOption(o, QString::number(v));}
	void addOption(const QString &o, double v) {addOption(o, QString::number(v));}
private:
	struct Data;
	QSharedDataPointer<Data> d;
//	static void cbEventManage(const libvlc_event_t *event, void *data);
//	void parseEvent(const libvlc_event_t *event);
};

#endif // VLCMEDIA_H

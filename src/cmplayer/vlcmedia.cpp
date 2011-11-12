#include "vlcmedia.hpp"
#include "libvlc.hpp"
#include <QtCore/QStringList>

struct VLCMedia::Data : public QSharedData {
	Data(const Mrl &mrl): mrl(mrl), media(0) {
		libvlc_instance_t *vlc = LibVLC::inst();
		if (mrl.isLocalFile())
			media = libvlc_media_new_path(vlc, mrl.toLocalFile().toLocal8Bit());
		else
			media = libvlc_media_new_location(vlc, mrl.toString().toLocal8Bit());


	}
	Data(const Data &rhs): QSharedData(rhs), mrl(rhs.mrl), media(0) {
		if (rhs.media)
			media = libvlc_media_duplicate(rhs.media);
	}
	~Data() {
		if (media)
			libvlc_media_release(media);
	}
	Mrl mrl;
	libvlc_media_t *media;
//	libvlc_event_manager_t *evMan;
};

VLCMedia::VLCMedia(const Mrl &mrl): d(new Data(mrl)) {
	if (!d->media) {
		LibVLC::outputError();
		return;
	}
//	d->evMan = libvlc_media_event_manager(d->m);
//	libvlc_event_type_t events[] = {
//		libvlc_MediaMetaChanged,
//	};
//	const int evCount = sizeof(events)/sizeof(*events);
//	for (int i=0; i<evCount; ++i)
//		libvlc_event_attach(d->evMan, events[i], cbEventManage, this);
}

VLCMedia::VLCMedia(const VLCMedia &rhs): d(rhs.d) {

}

VLCMedia &VLCMedia::operator = (const VLCMedia &rhs) {
	if (this != &rhs)
		d = rhs.d;
	return *this;
}

VLCMedia::~VLCMedia() {}

Mrl VLCMedia::mrl() const {
	return d->mrl;
}

const libvlc_media_t *VLCMedia::media() const {
	return d->media;
}

libvlc_media_t *VLCMedia::media() {
	return d->media;
}

//void VLCMedia::cbEventManage(const libvlc_event_t *event, void *data) {
//	reinterpret_cast<VLCMedia*>(data)->parseEvent(event);
//}

//void VLCMedia::parseEvent(const libvlc_event_t *event) {
//	switch (event->type) {
//	default:
//		break;
//	}
//}

void VLCMedia::addOption(const QStringList &opt) {
	QByteArray buffer;
	for (int i=0; i<opt.size(); ++i) {
		buffer = opt[i].toLocal8Bit();
		libvlc_media_add_option(d->media, buffer.constData());
	}
}

void VLCMedia::addOption(const QString &opt) {
	Q_ASSERT(d->media != 0);
	const QByteArray buffer = opt.toLocal8Bit();
	libvlc_media_add_option(d->media, buffer.data());
}



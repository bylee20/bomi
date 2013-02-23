#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"

struct MPContext;	struct vo_driver;
class VideoFormat;	class PlayEngine;
typedef quint32 uint32_t;	struct mp_image;
class VideoRendererItem;

class VideoOutput : public QObject {
	Q_OBJECT
public:
	VideoOutput(PlayEngine *engine);
	~VideoOutput();
	struct vo *vo_create(MPContext *mpctx);
	void prepare(void *avctx);
	void release();
	bool isHwAccActivated() const;
	const VideoFormat &format() const;
	void setRenderer(VideoRendererItem *renderer);
	static const vo_driver &getDriver();
	void quit();
signals:
	void formatChanged(const VideoFormat &format);
private slots:
	void handleFormatChanged(const VideoFormat &format);
private:
	static int preinit(struct vo *vo, const char *arg);
	static void uninit(struct vo */*vo*/) {}
	static int config(struct vo *vo, uint32_t w, uint32_t h, uint32_t , uint32_t , uint32_t , uint32_t fmt);
	static int control(struct vo *vo, uint32_t request, void *data);
	static void drawOsd(struct vo *vo, struct osd_state *osd);
	static void flipPage(struct vo *vo);
	static void checkEvents(struct vo */*vo*/) {}
	static int queryFormat(struct vo *vo, quint32 format);
	static void drawImage(struct vo *vo, mp_image *mpi);
	struct Data;
	Data *d;
};

#endif // VIDEOOUTPUT_HPP

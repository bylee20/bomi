#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"

struct MPContext;	struct vo_driver;
class VideoFormat;	class PlayEngine;
typedef quint32 uint32_t;	struct mp_image;
class VideoRendererItem;
class HwAcc;
class VideoOutput : public QObject {
	Q_OBJECT
public:
	VideoOutput(PlayEngine *engine);
	~VideoOutput();
	struct vo *vo_create(MPContext *mpctx);
	void prepare(void *avctx);
	void release();
	const VideoFormat &format() const;
	void setRenderer(VideoRendererItem *renderer);
	void quit();
	void output(const QImage &image);
	void setHwAcc(HwAcc *acc);
	HwAcc *hwAcc() const;
signals:
	void formatChanged(const VideoFormat &format);
	void reconfigured();
private:
	static int preinit(struct vo *vo);
	static void uninit(struct vo */*vo*/) {}
	static int reconfig(struct vo *vo, struct mp_image_params *params, int flags);
	static int control(struct vo *vo, uint32_t request, void *data);
	static void drawOsd(struct vo *vo, struct osd_state *osd);
	static void flipPage(struct vo *vo);
	static int queryFormat(struct vo *vo, quint32 format);
	static void drawImage(struct vo *vo, mp_image *mpi);
	struct Data;
	Data *d;
	friend vo_driver create_driver();
};

#endif // VIDEOOUTPUT_HPP

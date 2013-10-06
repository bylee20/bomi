#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"

// CAUTION: NEVER CALL THIS CLASS FROM Qt's GUI

struct MPContext;			struct vo_driver;
class VideoFormat;			class PlayEngine;
struct mp_image;			class DeintOption;
class VideoRendererItem;	class HwAcc;
enum class DeintMethod;

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
	void output(const QImage &image);
	void setHwAcc(HwAcc *acc);
	HwAcc *hwAcc() const;
	static int queryFormat(struct vo *vo, quint32 format);
	void setDeintOptions(const DeintOption &sw, const DeintOption &hw);
	void setDeintEnabled(bool on);
signals:
	void formatChanged(const VideoFormat &format);
private:
	void updateDeint();
	void reset();
	static int preinit(struct vo *vo);
	static void uninit(struct vo */*vo*/) {}
	static int reconfig(struct vo *out, struct mp_image_params *params, int flags);
	static int control(struct vo *vo, quint32 request, void *data);
	static void drawOsd(struct vo *vo, struct osd_state *osd);
	static void flipPage(struct vo *vo);
	static void drawImage(struct vo *vo, mp_image *mpi);
	static void getBufferedFrame(struct vo *vo, bool eof);
	struct Data;
	Data *d;
	friend vo_driver create_driver();
};

#endif // VIDEOOUTPUT_HPP

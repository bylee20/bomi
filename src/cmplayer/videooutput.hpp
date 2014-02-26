#ifndef VIDEOOUTPUT_HPP
#define VIDEOOUTPUT_HPP

#include "stdafx.hpp"
#include "videoformat.hpp"

// CAUTION: NEVER CALL THIS CLASS FROM Qt's GUI

struct vo_driver;
class VideoFormat;			class PlayEngine;
struct mp_image;			class DeintOption;
class VideoRendererItem;	class HwAcc;
enum class DeintMethod;

class VideoOutput : public QObject {
	Q_OBJECT
public:
	VideoOutput(PlayEngine *engine);
	~VideoOutput();
	void prepare(void *avctx);
	void release();
	const VideoFormat &format() const;
	void setRenderer(VideoRendererItem *renderer);
	void output(const QImage &image);
	void setHwAcc(HwAcc *acc);
	static int queryFormat(struct vo *vo, quint32 format);
signals:
	void formatChanged(VideoFormat format);
private slots:
	void setFrameRect(const QRectF &rect);
	void setTargetSize(const QSize &size);
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

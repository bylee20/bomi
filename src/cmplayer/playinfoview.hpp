//#ifndef PLAYINFOVIEW_HPP
//#define PLAYINFOVIEW_HPP

//#include "stdafx.hpp"

//class VideoFormat;		class OsdRenderer;
//class VideoRendererItem;		class AudioFormat;
//class AudioController;		class PlayEngine;

//class PlayInfoView : public QObject {
//	Q_OBJECT
//public:
//	PlayInfoView(const PlayEngine *engine, const AudioController *audio, const VideoRendererItem *video);
//	~PlayInfoView();
//	OsdRenderer &osd() const;
//public slots:
//	void setVisible(bool visible);
//private slots:
//	void onTick(int ms);
//	void onVideoFormatChanged(const VideoFormat &vfmt);
//	void setAudioFormat(const AudioFormat &afmt);
//	void onAboutToPlay();
//	void updateResourceUsage();
//private:
//	typedef QLatin1String _L;
//	static auto _8(const char *str, int len = -1) -> QString {return QString::fromLocal8Bit(str, len);}
////	static auto _n(int n, int base = 10) -> QString {return QString::number(n, base);}
//	auto bps(int Bps) -> QString {return (Bps ? _n(Bps*8/1000) % _L("kbps") : QString());}
//	static auto format(quint32 fmt) -> QString {return fmt >= 0x20202020 ? _8((const char*)&fmt, 4) : _L("0x") % _n(fmt, 16);}
//	static auto resolution(int w, int h) -> QString {return _n(w) % _L("x") % _n(h);}
//	struct Data;
//	Data *d;
//};

//#endif // PLAYINFOVIEW_HPP

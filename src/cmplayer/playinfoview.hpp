#ifndef PLAYINFOVIEW_HPP
#define PLAYINFOVIEW_HPP

#include <QtCore/QObject>

class VideoFormat;		class OsdRenderer;
class VideoRenderer;		class AudioFormat;
class AudioController;

class PlayInfoView : public QObject {
	Q_OBJECT
public:
	PlayInfoView(QObject *parent = 0);
	~PlayInfoView();
	OsdRenderer *osd() const;
	void setVideo(const VideoRenderer *video);
	void setAudio(const AudioController *audio);
public slots:
	void setVisible(bool visible);
private slots:
	void update();
	void setVideoFormat(const VideoFormat &vfmt);
	void setAudioFormat(const AudioFormat &afmt);
	void setProcInfo(double cpu, int rss, double mem);
private:
	struct Data;
	Data *d;
};

#endif // PLAYINFOVIEW_HPP

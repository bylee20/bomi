#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

#include "stdafx.hpp"

struct af_instance;		struct mp_audio;
struct af_cfg;			struct af_info;
struct mp_chmap;
class ChannelLayoutMap;
enum class ClippingMethod;
enum class ChannelLayout;
struct AudioNormalizerOption;

class AudioController : public QObject {
	Q_OBJECT
public:
	AudioController(QObject *parent = nullptr);
	~AudioController();
	void setAmpLevel(double level);
	void setNormalizerActivated(bool on);
	void setTempoScalerActivated(bool on);
	double gain() const;
	bool isTempoScalerActivated() const;
	bool isNormalizerActivated() const;
	void setNormalizerOption(double length, double target, double silence, double min, double max);
	double scale() const;
	void setMuted(bool muted);
	void setClippingMethod(ClippingMethod method);
	void setChannelLayoutMap(const ChannelLayoutMap &map);
	void setOutputChannelLayout(ChannelLayout layout);
	mp_chmap *chmap() const;
signals:
	void started();
private:
	static int open(af_instance *af);
	static bool test(int fmt_in, int fmt_out);
	int reinitialize(mp_audio *data);
	static int filter(af_instance *af, mp_audio *data, int flags);
	static void uninit(af_instance *af);
	static int control(af_instance *af, int cmd, void *arg);
	struct Data;
	Data *d;
	friend af_info create_info();
};

#endif // AUDIOCONTROLLER_HPP

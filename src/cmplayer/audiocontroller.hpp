#ifndef AUDIOCONTROLLER_HPP
#define AUDIOCONTROLLER_HPP

#include "stdafx.hpp"

struct af_instance;
struct mp_audio;
struct af_cfg;

class AudioController : public QObject {
	Q_OBJECT
public:
	AudioController(QObject *parent = nullptr);
	~AudioController();
	void setVolume(double volume);
	double volume() const;
	bool setNormalizer(bool on);
	bool setScaletempo(bool on);
	double normalizer() const;
	bool scaletempo() const;
	void setNormalizer(double target, double silence, double min, double max);
private:
	static int init(af_instance *af, const af_cfg *cfg);
	int config(mp_audio *data);
	static mp_audio *play(af_instance *af, mp_audio *data);
	static void uninit(af_instance *af);
	static int control(af_instance *af, int cmd, void *arg);
	struct Data;
	Data *d;
	friend int ac_open(af_instance *af);
};

#endif // AUDIOCONTROLLER_HPP

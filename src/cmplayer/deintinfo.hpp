#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "stdafx.hpp"
#include "enums.hpp"

struct DeintOption {
	DeintOption() {}
	DeintOption(DeintMethod method, DeintDevice device, bool doubler)
	: method(method), device(device), doubler(doubler) {}
	bool operator == (const DeintOption &rhs) const {
		return method == rhs.method && doubler == rhs.doubler && device == rhs.device;
	}
	bool operator != (const DeintOption &rhs) const { return !operator == (rhs); }
	QString toString() const;
	static DeintOption fromString(const QString &string);
// variables
	DeintMethod method = DeintMethod::None;
	DeintDevice device = DeintDevice::CPU;
	bool doubler = false;
};

class DeintCaps {
public:
	static QList<DeintCaps> list();
	DeintMethod method() const { return m_method; }
	bool hwdec() const { return supports(DecoderDevice::GPU); }
	bool swdec() const { return supports(DecoderDevice::CPU); }
	bool doubler() const { return m_doubler; }
	bool supports(DeintDevice dev) const { return m_device & dev; }
	bool supports(DecoderDevice dev) const { return m_decoder & dev; }
	bool isAvailable() const { return m_method != DeintMethod::None && m_device != 0 && m_decoder != 0; }
	QString toString() const;
	static DeintCaps fromString(const QString &text);
	static DeintCaps default_(DecoderDevice dec);
private:
	friend class DeintWidget;
	DeintMethod m_method = DeintMethod::None;
	int m_decoder = 0, m_device = 0;
	bool m_doubler = false;
};



class DeintWidget : public QWidget {
	Q_OBJECT
public:
	DeintWidget(DecoderDevice decoder, QWidget *parent = nullptr);
	~DeintWidget();
	void set(const DeintCaps &caps);
	DeintCaps get() const;
	static QString informations();
private:
	static constexpr auto GPU = DeintDevice::GPU;
	static constexpr auto CPU = DeintDevice::CPU;
	static constexpr auto OpenGL = DeintDevice::OpenGL;
	struct Data;
	Data *d;
};

#endif // DEINTINFO_HPP

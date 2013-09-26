#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "stdafx.hpp"
#include "enums.hpp"

class DeintInfo {
	Q_DECLARE_TR_FUNCTIONS(DeintInfo)
public:
	bool operator == (const DeintInfo &rhs) const {
		return m_method == rhs.m_method && m_flags == rhs.m_flags;
	}
	bool operator != (const DeintInfo &rhs) const { return !operator==(rhs); }
	enum Device {
		DeviceMask     = 0x0000ff,
		PostProc       = 0x000001,
		OpenGL         = 0x000002,
		VaApi          = 0x000004,
		AvFilter       = 0x000008,
		Software       = PostProc | AvFilter,
		Hardware       = OpenGL
#ifdef Q_OS_LINUX
		| VaApi
#endif
	};
	enum FrameRate {
		FrameRateMask  = 0x00ff00,
		SingleRate     = 0x000100,
		DoubleRate     = 0x000200
	};
	enum Method { Bob, LinearBob, CubicBob, LinearBlend, Median, Yadif, MethodCount };
	static QList<DeintInfo> list();
	DeintInfo(Method method, int flags);
	int capabilities() const;
	QString name() const;
	static QString description(Method method);
	static QString name(Method method);
	static int capabilities(Method method);
	int flags() const { return m_flags; }
	int hardware() const { return m_flags & Hardware; }
	Method method() const { return m_method; }
	DeintInfo() = default;
	bool isDoubleRate() const { return m_flags & DoubleRate; }
	bool isSingleRate() const { return m_flags & SingleRate; }
	bool isHardware() const { return m_flags & Hardware; }
	bool isSoftware() const { return m_flags & Software; }
	bool hasFlags() const { return m_flags != 0; }
	bool hasFlags(bool flags) const { return m_flags & flags; }
	QString toString() const { return name() + "|" + QString::number(m_flags); }
	static DeintInfo fromString(const QString &text);
	static QList<DeintInfo> hwdecList();
	static QList<DeintInfo> swdecList();
private:
	struct Item;
	Method m_method = Bob;
	int m_flags = 0;
};

class DeintWidget : public QWidget {
	Q_OBJECT
public:
	DeintWidget(const DeintInfo &info, bool hwdec, QWidget *parent = nullptr);
	~DeintWidget();
	void setInfo(const DeintInfo &info);
	DeintInfo info() const;
	DeintInfo::Method method() const;
private:
	struct Data;
	Data *d;
};

#endif // DEINTINFO_HPP

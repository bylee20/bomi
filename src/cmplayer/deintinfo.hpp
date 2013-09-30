#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "stdafx.hpp"
#include "enums.hpp"

struct DeintOption {
	bool operator == (const DeintOption &rhs) const {
		return method == rhs.method && doubler == rhs.doubler && hwacc == rhs.hwacc;
	}
	bool operator != (const DeintOption &rhs) const { return !operator == (rhs); }
	DeintMethod method = DeintMethod::None;
	bool doubler = false, hwacc = false;
	QString toString() const;
	static DeintOption fromString(const QString &string);
	static DeintOption default_(DeintMethod method);
};

class DeintWidget : public QWidget {
	Q_OBJECT
public:
	DeintWidget(bool hwdec, QWidget *parent = nullptr);
	~DeintWidget();
	void setOption(const DeintOption &info);
	DeintOption option() const;
private:
	struct Data;
	Data *d;
};

#endif // DEINTINFO_HPP

#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "stdafx.hpp"

#define CONNECT(a, b, c, d) (QObject::connect(a, SIGNAL(b), c, SLOT(d)))

namespace Global {

template<typename T>
struct Range {
	Range() {}
	Range(const T &min, const T &max): min(min), max(max) {}
	bool contains(const T &t) {return min <= t && t <= max;}
	bool isValid() const {return min <= max;}
	T difference() const {return max - min;}
	T min = 0, max = 0;
};
typedef Range<double> RangeF;
typedef Range<int> RangeI;

struct CharArrayList {
	CharArrayList() {}
	CharArrayList(const QStringList &list) {
		m_list.resize(list.size());
		m_arrays.resize(list.size());
		for (int i=0; i<list.size(); ++i) {
			m_arrays[i] = list[i].toLocal8Bit();
			m_list[i] = m_arrays[i].data();
		}
	}
	char **data() {return m_list.data();}
	char *const* data() const {return m_list.data();}
	bool contains(const char *string) {
		for (int i=0; i<m_list.size(); ++i) {
			if (qstrcmp(m_list[i], string) == 0)
				return true;
		}
		return false;
	}
	bool isEmpty() const {return m_list.isEmpty();}
	int size() const {return m_list.size();}
	void append(const char *string) {
		m_arrays.append(QByteArray(string));
		m_list.append(m_arrays.last().data());

	}
	void clear() {m_list.clear(); m_arrays.clear();}
private:
	QVector<char *> m_list;
	QVector<QByteArray> m_arrays;
};


enum EngineState {EngineStopped = 1, EnginePlaying = 2, EnginePaused = 4, EngineFinished = 8, EngineLoading = 16, EngineError = 64};
enum StreamType {UnknownStream = 0, VideoStream, AudioStream, SubPicStream};
enum MediaMetaData {LanguageCode};

static inline QString toString(int value, bool sign) {
	if (!sign || value < 0) return QString::number(value);
	return (value > 0 ? _L("+") : _U("±")) += QString::number(value);
}

static inline QString toString(double value, bool sign, int n = 1) {
	if (n <= 0)
		return toString(qRound(value), sign);
	QString ret;
	if (sign && value >= 0)
		ret = (value > 0 ? _L("+") : _U("±"));
	QByteArray fmt("%.");	fmt.reserve(10);
	fmt.append(QByteArray::number(n)).append("f");
	return ret += QString().sprintf(fmt.data(), value);
}

static inline QString toString(const QSize &size) {
	QString ret = QString::number(size.width()); ret.reserve(ret.size()*2 + 5);
	ret += QString::fromUtf8("\303\227"); ret += QString::number(size.height());
	return ret;
}

static inline QPointF toPointF(const QSizeF &size) {return QPointF(size.width(), size.height());}

QDialogButtonBox *makeButtonBox(QDialog *dlg);
}

using namespace Global;

#endif // GLOBAL_HPP

#ifndef RICHSTRING_HPP
#define RICHSTRING_HPP

#include <QtCore/QString>

class RichString {
public:
	static QStringRef ref(const QString &str) {return str.midRef(0);}
	static QStringRef trimmed(const QStringRef &ref);
	static QStringRef processEntity(int &idx, const QStringRef &ref);
	static bool skipSeperator(int &idx, const QStringRef &text);
	static inline QStringRef midRef(const QStringRef &ref, int from, int n = -1) {
		return ref.string()->midRef(ref.position() + from, n < 0 ? ref.size() - from : n);
	}
	static inline bool isRightBracket(ushort c) {return c == '>';}
	static inline bool isSeperator(ushort c) {return c == ' ' || c == '\t' || c == '\r' || c== '\n';}
	static inline bool inRange(ushort min, ushort c, ushort max) {return min <= c && c <= max;}
	static inline bool isNumber(ushort c) {return inRange('0', c, '9');}
	static inline bool isAlphabet(ushort c) {return inRange('a', c, 'z') || inRange('A', c, 'Z');}
	static inline bool isHexNumber(ushort c) {return isNumber(c) || inRange('a', c, 'f') || inRange('A', c, 'F');}
	RichString();
	RichString(const QString &rich, const QString &plain);
	RichString(const QStringRef &ref);
	RichString(const QString &string);
	RichString(const RichString &other);
	~RichString();
	RichString &operator = (const RichString &rhs);
	QString toString() const {return m_rich;}
	QString toPlain() const {return m_plain;}
	bool isEmpty() const;
	bool hasWords() const;
	int size() const;
	void clear();
	RichString &merge(const RichString &other);
	RichString merged(const RichString &other) const;
	static void process(const QStringRef &ref, QString &rich, QString &plain, bool hasTag = true);
private:
	void cache(const QStringRef &ref);
	QString m_rich, m_plain;
};

#endif // RICHSTRING_HPP

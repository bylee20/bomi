//#ifndef RICHTEXT_HPP
//#define RICHTEXT_HPP

//#include <QtGui/QTextLayout>
//#include <set>

//class RichTextLines;		class QPainter;
//class RichTextRuby;

//struct RichTextLines {
//	void setLayout(const QString &text, const QList<QTextLayout::FormatRange> &format) {
//		layout.setText(text);
//		layout.setAdditionalFormats(format);
//	}
//	~RichTextLines() {qDeleteAll(ruby);}
//	QTextLayout layout;
//	QVector<RichTextRuby*> ruby;
//};

//class RichTextParser {
//	typedef QTextCharFormat Format;
//	typedef Format::Property Property;
//	typedef std::set<Property> PropSet;
//public:
//	static const int Overwritten = QTextFormat::UserProperty+1;
//	static inline QStringRef midRef(const QStringRef &ref, int from, int n = -1) {
//		return ref.string()->midRef(ref.position() + from, n < 0 ? ref.size() - from : n);
//	}
//	static inline bool isRightBracket(ushort c) {return c == '>';}
//	static inline bool isSeperator(ushort c) {return c == ' ' || c == '\t' || c == '\r' || c== '\n';}
//	static inline bool inRange(ushort min, ushort c, ushort max) {return min <= c && c <= max;}
//	static inline bool isNumber(ushort c) {return inRange('0', c, '9');}
//	static inline bool isAlphabet(ushort c) {return inRange('a', c, 'z') || inRange('A', c, 'Z');}
//	static inline bool isHexNumber(ushort c) {return isNumber(c) || inRange('a', c, 'f') || inRange('A', c, 'F');}
//	static inline bool skipSeperator(int &pos, const QStringRef &text) {
//		for (; pos < text.size() && isSeperator(text.at(pos).unicode()); ++pos) ;
//		return pos >= text.size(); // true for end
//	}
//	static QStringRef trim(const QStringRef &text);

//	~RichTextParser();
//	bool setText(const QString &text);
//	void updateLayout(double maxWidth, double leading);
//	void updateLayout(double maxWidth);
//	void clear();
//	void draw(QPainter *painter, const QPointF &pos = QPointF(0, 0));
//	void setDefaultFormat(const QTextCharFormat &format) {m_format = format;}
//	void setDefaultOption(const QTextOption &option) {m_option = option;}
//	QTextOption defaultOption() const {return m_option;}
//	QTextOption &defaultOption() {return m_option;}
//	QTextCharFormat &defaultFormat() {return m_format;}
//	QTextCharFormat defaultFormat() const {return m_format;}
//	QSizeF size() const {return m_size;}
//	QSizeF naturalSize() const {return m_natural;}
//	void applyDefaultFormat();
//	inline void setProperty(QTextFormat::Property p, const QVariant &var) {m_format.setProperty(p, var);}
//	void copyFrom(const RichTextParser &other);
//private:
//	static inline bool same(const QStringRef &str1, const char *str2) {return !str1.compare(QLatin1String(str2), Qt::CaseInsensitive);}
//	inline ushort at(int pos) const {return m_ref.at(pos).unicode();}
//	inline bool isAt(int pos, ushort c) const {return m_ref.at(pos).unicode() == c;}
//	int setFormat(const QStringRef &tag, const QStringRef &attr, const QStringRef &value, Format &format);
//	int setFormat(const QStringRef &tag, Format &format);
//	QStringRef parseTag(int &pos, Format &format, int &overwritten);

//	enum Style {
//		ForegroundColor = 1,
//		FontPixelSize = 2,
//		FontWeight = 4,
//		FontFamily = 8,
//		FontItalic = 16,
//		FontUnderline = 32,
//		FontStrikeOut = 64
//	};
//	QString m_text;
//	QStringRef m_ref;
//	QVector<QStringRef> m_tag;
//	QVector<RichTextLines*> m_lines;
//	QVector<QTextCharFormat> m_trace;
//	QTextCharFormat m_format;
//	QTextOption m_option;
//	QSizeF m_size, m_natural;
//};

//#endif // RICHTEXT_HPP

//#include "richtext.hpp"
//#include <QtCore/QDebug>
//#include <QtGui/QFont>
//#include "tagiterator.hpp"
//#include <QtGui/QPainter>

//#include <QTextDocument>
//#include "richtext.hpp"
//#include <list>
//#include <QtGui/QPen>
//#include <QtGui/QBrush>
//#include <QtGui/QColor>
//#include <QtGui/QPainter>

////class NodeContext {
////public:
////	NodeContext() {
////		m_bold = 0;
////		m_pen = 0;
////	}
////	~NodeContext() {
////		delete m_bold;
////		delete m_pen;
////	}
////	NodeContext(const NodeContext &rhs) {
////		m_bold = rhs.m_bold ? new bool(*rhs.m_bold) : 0;
////		m_pen = rhs.m_pen ? new QPen(*rhs.m_pen) : 0;
////	}
////	NodeContext &operator=(const NodeContext &rhs) {
////		if (this != &rhs) {
////			copy(m_bold, rhs.m_bold);
////			copy(m_pen, rhs.m_pen);
////		}
////		return *this;
////	}
////	void apply(QFont &font) {
////		if (m_bold)
////			font.setBold(*m_bold);
////	}

////	void apply(QPainter *painter) {
////		if (m_pen)
////			painter->setPen(*m_pen);
////	}
////#define DEC_SETTER(type, name, Name) void set##Name(type name) {if (!m_##name) {m_##name = new type(name);} else{*m_##name = name;}}
////	DEC_SETTER(bool, bold, Bold)
////	DEC_SETTER(QPen, pen, Pen)
////	template<typename T>
////	static inline void copy(T *&dest, const T *src) {
////		if (src) {
////			if (!dest)
////				dest = new T(*src);
////			else
////				*dest = *src;
////		} else if (dest) {
////			delete dest;
////			dest = 0;
////		}
////	}

////private:
////	bool *m_bold;
////	QPen *m_pen;
////};



////struct Node {
//////	QSizeF sizeHint(const QFont &font) const {
//////		if ()
//////		QFontMetricsF
//////	}
////	bool hasTag() const {return !tag.elem.isEmpty();}

////	struct Tag {
////		Tag(): context(0) {}
////		~Tag() {delete context;}
////		Tag(const Tag &rhs): elem(rhs.elem), klass(rhs.klass), id(rhs.id) {
////			context = rhs.context ? new NodeContext(*rhs.context) : 0;
////		}
////		Tag &operator = (const Tag &rhs) {
////			if (this != &rhs) {
////				elem = rhs.elem;
////				klass = rhs.klass;
////				id = rhs.id;
////				NodeContext::copy(context, rhs.context);
////			}
////			return *this;
////		}

////		bool hasEnd() const {
////			return elem == QLatin1String("p") || elem == QLatin1String("span") || elem == QLatin1String("font") || elem == QLatin1String("sync");
////		}
////		QColor makeColor(const QStringRef &value) {
////			QString name;
////			for (int i=0; i<value.size(); ++i) {
////				const ushort ucs = value.at(i).unicode();
////				if (!(('0' <= ucs && ucs <= '9') || ('a' <= ucs && ucs <= 'f') || ('A' <= ucs && ucs <= 'F'))) {
////					name = value.toString();
////					break;
////				}
////			}
////			if (name.isEmpty()) {
////				name += QLatin1Char('#');
////				name += value;
////			}
////			return QColor(name);
////		}

////		void addAttribute(const QStringRef &name, const QStringRef &value) {
////#define IS_SAME(str) (name.compare(QLatin1String(str), Qt::CaseInsensitive) == 0)
////			if (IS_SAME("class"))
////				klass = name.toString();
////			else if (IS_SAME("id"))
////				id = name.toString();
////			else {
////				if (!context)
////					context = new NodeContext;
////				if (IS_SAME("color"))
////					context->setPen(makeColor(value));
////			}
////		}
////		void applyContext(QPainter *painter, int &depth) {
////			if (elem.startsWith(QLatin1Char('/'))) {
////				if (depth > 0) {
////					painter->restore();
////					--depth;
////				}
////			} else if (context) {
////				painter->save();
////				context->apply(painter);
////				++depth;
////			}
////		}
////		QString elem;
////		QString klass;
////		QString id;
////	private:
////		NodeContext *context;
////	};
////	Tag tag;
////	QString text;
////};

////struct Line {
////	Line() {nodes.append(Node());}
////	QList<Node> nodes;
////};

////class Parser {
////public:
////	QList<Line> parse(const QStringRef &source) {
////		src = source;
////		pos = 0;
////		lines.clear();
////		lines.append(Line());
////		std::list<QStringRef> pair;
////		while (!atEnd()) {
////			const QChar c = src.at(pos);
////			if (c.unicode() == '<') {
////				if (!parseTag())
////					continue;
////				const Node::Tag &tag = lines.last().nodes.last().tag;
////				if (tag.hasEnd())
////					pair.push_front(tag.elem.midRef(0));
////				else if (!tag.elem.isEmpty() && tag.elem.at(0) == '/') {
////					std::list<QStringRef>::reverse_iterator it;
////					for (it=pair.rbegin(); it != pair.rend(); ++it) {
////						if (it->compare(tag.elem.midRef(1), Qt::CaseInsensitive) == 0) {
////							pair.erase(--it.base());
////							break;
////						}
////					}
////				}
////			} else {
////				if (lines.last().nodes.last().hasTag())
////					lines.last().nodes.append(Node());
////				lines.last().nodes.last().text += c;
////				++pos;
////			}
////		}
////		std::list<QStringRef>::const_reverse_iterator it = pair.rbegin();
////		for (; it!=pair.rend(); ++it) {
////			lines.last().nodes.push_back(Node());
////			QString &elem = lines.last().nodes.last().tag.elem;
////			elem.reserve(1 + it->size());
////			elem += QLatin1Char('/');
////			elem += *it;
////		}
////		return lines;
////	}
////private:
////	bool check(int at, ushort ucs) const {return at < src.size() && same(at, ucs);}
////	bool same(int at, ushort ucs) const {return src.at(at).unicode() == ucs;}
////	QStringRef midRef(int from, int count = -1) const {return src.string()->midRef(from, count);}
////	QStringRef parseWord() {
////		if (ucs() == '\"' || ucs() == '\'') {
////			const ushort q = this->ucs();
////			const int from = ++pos;
////			int count = 0;
////			while (!atEnd()) {
////				const ushort ucs = this->ucs();
////				++pos;
////				if (ucs == q)
////					break;
////				++count;
////			}
////			return midRef(from, count);
////		} else {
////			const int from = pos;
////			int count = 0;
////			while (!atEnd()) {
////				const ushort ucs = this->ucs();
////				if (isspace(ucs) || ucs == '>' || ucs == '=')
////					break;
////				if (ucs == '/' && check(pos+1, '>'))
////					break;
////				++pos;
////				++count;
////			}
////			return midRef(from, count);
////		}
////	}
////	bool same(const QStringRef &lhs, const char *rhs) const {
////		const int len = strlen(rhs);
////		if (lhs.length() != len)
////			return false;
////		for (int i=0; i<len; ++i) {
////			if (lhs.at(i).unicode() != rhs[i])
////				return false;
////		}
////		return true;
////	}
////	bool same(const QString &lhs, const char *rhs) const {
////		const int len = strlen(rhs);
////		if (lhs.length() != len)
////			return false;
////		for (int i=0; i<len; ++i) {
////			if (lhs.at(i).unicode() != rhs[i])
////				return false;
////		}
////		return true;
////	}
////	static void appendAsLower(QString &dst, const QStringRef &src) {
////		dst.reserve(dst.size() + src.size());
////		for (int i=0; i<src.size(); ++i)
////			dst += src.at(i).toLower();
////	}
////	QChar qchar() const {return src.at(pos);}
////	ushort ucs() const {return src.at(pos).unicode();}
////	bool atEnd() const {return pos >= src.length();}
////	bool skipSpace() {for (; !atEnd() && isspace(src.at(pos).unicode()); ++pos) ; return atEnd();}
////	bool parseTag() {
////		Q_ASSERT(ucs() == '<');
////		++pos;
////		if (skipSpace())
////			return false;
////		const QStringRef elem = parseWord();
////		if (same(elem, "!--")) {
////			pos += 3;
////			while (!atEnd()) {
////				if (same(pos, '>') && same(pos-1, '-') && same(pos-2, '-')) {
////					++pos;
////					break;
////				}
////				++pos;
////			}
////			return false;
////		} else if (elem.isEmpty())
////			return false;
////		if (same(elem, "br")) {
////			lines.append(Line());
////			return false;
////		}
////		lines.last().nodes.append(Node());
////		Node::Tag &tag = lines.last().nodes.last().tag;
////		appendAsLower(tag.elem, elem);
////		while (!atEnd()) {
////			if (skipSpace())
////				return true;
////			if (ucs() == '>') {
////				++pos;
////				return true;
////			}
////			const QStringRef name = parseWord();
////			if (skipSpace())
////				return true;
////			if (ucs() == '=') {
////				++pos;
////				if (skipSpace())
////					return true;
////				tag.addAttribute(name, parseWord());
//////				attr.value = parseWord().toString();
////			}
////		}
////		return true;
////	}

////	int pos;
////	QStringRef src;
//////	QList<Node> nodes;
////	QList<Line> lines;
////};

////void draw(QPainter *painter, const QList<Node> &nodes) {
////	int depth = 0;
////	for (int i=0; i<nodes.size(); ++i) {
////		const Node &node = nodes[i];
////		if (node.hasTag()) {

////		}
////	}
////}


////class Text {
////public:
////	void setHtml(const QString &html) {
////		Parser p;
////		m_lines = p.parse(html.midRef(0));
////	}
////	QSizeF sizeHint() const {

////	}
////private:
////	QList<Line> m_lines;
////};


////class Block {
////public:
////	virtual ~Block() {qDeleteAll(m_children);}
////	virtual void appendHtml(QString &html) const = 0;
////	virtual bool isBlock() const = 0;
////protected:
////	static bool needToAddSharp(const QStringRef &name, const QStringRef &value) {
////		if (value.size() != 6)
////			return false;
////		if (!TagIterator::opEq(name, "color"))
////			return false;
////		for (int i=0; i<6; ++i) {
////			const ushort ucs = value.at(i).unicode();
////			if (!RichString::isHexNumber(ucs))
////				return false;
////		}
////		return true;
////	}
////	static void appendAttr(QString &html, const TagIterator::Attr &attr) {
////		if (attr.name.isEmpty())
////			return;
////		html += QLatin1Char(' ');
////		html += attr.name;
////		if (!attr.value.isEmpty()) {
////			html += QLatin1Char('=');
////			if (attr.q.unicode() != '\0')
////				html += attr.q;
////			if (needToAddSharp(attr.name, attr.value))
////				html += QLatin1Char('#');
////			html += attr.value;
////			if (attr.q.unicode() != '\0')
////				html += attr.q;
////		}
////	}

////	static inline QStringRef midRef(const QStringRef &ref, int from, int n = -1) {
////		return ref.string()->midRef(ref.position() + from, n);
////	}
////	static inline bool skipSpaces(int &pos, const QStringRef &ref) {
////		if (pos < 0)
////			return false;
////		bool ret = false;
////		for (; pos < ref.size(); ++pos) {
////			const ushort ucs = ref.at(pos).unicode();
////			if (ucs == '\n' || ucs == '\r' || ucs == '\v')
////				continue;
////			if (ucs == ' ' || ucs == '\t') {
////				ret = true;
////				continue;
////			}
////			return ret;
////		}
////		return ret;
////	}
////	QList<Block*> m_children;
////};


////struct TextBlock : public Block {
////	bool isBlock() const {return false;}
////	static TextBlock *make(const QStringRef &text) {
////		TextBlock *block = new TextBlock;
////		block->m_text = text;
////		return block;
////	}
////	void appendHtml(QString &html) const {
////		for (int pos = 0; pos <m_text.size(); ++pos) {
////			const ushort ucs = m_text.at(pos).unicode();
////			if (ucs == '\n' || ucs == '\r' || ucs == '\v')
////				continue;
////			if (ucs == ' ' || ucs == '\t') {
////				static const QChar spc(' ');
////				if (!html.endsWith(spc))
////					html += spc;
////			} else
////				html += ucs;
////		}
////	}
////private:
////	QStringRef m_text;
////};

////void process(QString &html, QString &plain, const QStringRef &text, TagIterator &tag) {

////}

////class TagBlock : public Block {
////public:
////	TagBlock() {m_pair = false; m_block = false;}
////	static TagBlock *make(const QStringRef &text, TagIterator &tag) {
////		if (tag.begin() < 0)
////			return 0;
////		TagBlock *block = new TagBlock;
////		block->m_pair = false;
////		block->m_block = isBlock(tag);
////		block->m_elem = tag.element();
////		const int acount = tag.attributeCount();
////		for (int i=0; i<acount; ++i)
////			block->m_attr.append(tag.attribute(i));
////		if (const char *pair = pairTag(tag)) {
////			block->m_pair = true;
////			for (;;) {
////				const int prev = tag.pos();
////				const int begin = tag.next();
////				if (begin < 0) {
////					if (TextBlock *b = TextBlock::make(midRef(text, prev)))
////						block->m_children.append(b);
////					return block;
////				}
////				if (prev < begin) {
////					if (TextBlock *b = TextBlock::make(midRef(text, prev, begin-prev)))
////						block->m_children.append(b);
////				}
////				if (tag.elementIs(pair))
////					return block;
////				if (tag.elementIs("sync") || tag.elementIs("/body"))
////					return block;
////				if (TagBlock *b = TagBlock::make(text, tag))
////					block->m_children.append(b);
////			}
////		} else {
////			return block;
////		}
////	}
////	void appendHtml(QString &html) const {
////		html += QLatin1Char('<');
////		html += m_elem;
////		for (int i=0; i<m_attr.size(); ++i) {
////			html += QLatin1Char(' ');
////			html += m_attr[i].name;
////			if (!m_attr[i].value.isEmpty()) {
////				html += QLatin1String("=\"");
////				html += m_attr[i].value;
////				html += '"';
////			}
////		}
////		html += QLatin1Char('>');
////		for (int i=0; i<m_children.size(); ++i) {
////			m_children[i]->appendHtml(html);
////		}
////		if (m_pair) {
////			html += QLatin1String("</");
////			html += m_elem;
////			html += QLatin1Char('>');
////		}
////		html += QLatin1Char('\n');
////	}
////	void appendPlain(QString &plain) const {

////	}

////	bool isBlock() const {return m_block;}
////private:
////	static bool isBlock(const TagIterator &tag) {
////		return tag.elementIs("p");
////	}
////	static const char *pairTag(const TagIterator &tag) {
////#define CHECK_TAG(elem) if (tag.elementIs(elem)) {return "/"elem;}
////		CHECK_TAG("p");
////		CHECK_TAG("sync");
////		CHECK_TAG("font");
////		CHECK_TAG("span");
////#undef CHECK_TAG
////		return 0;
////	}
////	QStringRef m_elem;
////	QList<TagIterator::Attr> m_attr;
////	bool m_pair, m_block;
////};

////class SyncBlock : public Block {
////	QList<TagIterator::Attr> m_attr;
////public:
////	bool isBlock() const {return true;}
////	static SyncBlock *make(const QStringRef &text, TagIterator &tag) {
////		if (!tag.elementIs("sync"))
////			return 0;
////		SyncBlock *sync = new SyncBlock;
////		const int acount = tag.attributeCount();
////		for (int i=0; i<acount; ++i)
////			sync->m_attr.append(tag.attribute(i));
////		for (;;) {
////			const int prev = tag.pos();
////			const int begin = tag.next();
////			if (begin < 0) {
////				if (TextBlock *b = TextBlock::make(midRef(text, prev)))
////					sync->m_children.append(b);
////				return sync;
////			}
////			if (prev < begin) {
////				if (TextBlock *b = TextBlock::make(midRef(text, prev, begin-prev)))
////					sync->m_children.append(b);
////			}
////			if (tag.elementIs("/sync") || tag.elementIs("sync") || tag.elementIs("/body"))
////				return sync;
////			if (TagBlock *b = TagBlock::make(text, tag))
////				sync->m_children.append(b);
////		}
////	}
////	void appendHtml(QString &html) const {
////		html += QLatin1String("<span");
////	}
////};


////struct RootBlock : public Block {
////	void draw(QPainter *painter, const QRectF &rect) {}
////	bool isBlock() const {return true;}
////	QString toHtml() const {
////		QString html;
////		appendHtml(html);
////		return html;
////	}
////	void appendHtml(QString &html) const {
////		for (int i=0; i<m_children.size(); ++i)
////			m_children[i]->appendHtml(html);
////	}
////	static RootBlock *make(const QString &text, const QFont &font, const QPen &pen) {
////		RootBlock *root = new RootBlock;
////		TagIterator tag(text);
////		tag.setPos(0);
////		for (;;) {
////			const int prev = tag.pos();
////			const int begin = tag.next();
////			if (begin < 0) {
////				if (TextBlock *block = TextBlock::make(text.midRef(prev), font))
////					root->m_children.append(block);
////				return root;
////			}
////			if (prev < begin) {
////				if (TextBlock *block = TextBlock::make(text.midRef(prev, begin-prev), font))
////					root->m_children.append(block);
////			}
////			if (TagBlock *block = TagBlock::make(text.midRef(0), font, pen, tag))
////				root->m_children.append(block);
////		}
////		return root;
////	}
////};

//struct RichText::Data {
////	QFont font;
////	RootBlock *root;
//};

//struct Node {
//	struct Tag {
//		struct Attr {
//			void clear() {name.clear(); value.clear();}
//			QString name, value;
//		};
//		QString elem;
//		QList<Attr> attr;
//	};
//	Tag tag;
//	QString text;
//};

//class Parser {
//public:
//	QList<Node> parse(const QStringRef &source) {
//		src = source;
//		pos = 0;
//		nodes.clear();
//		nodes.append(Node());
//		while (!atEnd()) {
//			const QChar c = src.at(pos++);
//			if (c.unicode() == '<') {
//				parseTag();
//			} else {
//				nodes.last().text += c;
//			}
//		}
//		return nodes;
//	}
//private:
//	QStringRef midRef(int from, int count = -1) {return src.string()->midRef(from, count);}

//	QStringRef parseWord() {
//		if (ucs() == '\"' || ucs() == '\'') {
//			const ushort q = src.at(pos++).unicode();
//			const int from = pos;
//			int count = 0;
//			while (!atEnd()) {
//				const ushort ucs = src.at(pos++).unicode();
//				if (ucs == q) {
//					++pos;
//					break;
//				}
//				++count;
//			}
//			return midRef(from, count);
//		} else {
//			const int from = pos;
//			int count = 0;
//			while (!atEnd()) {
//				const ushort ucs = src.at(pos++).unicode();
//				if (isspace(ucs) || ucs == '>' || ucs == '=')
//					break;
//				if (ucs == '/' && pos+1 < src.size() && src.at(pos+1).unicode() == '>')
//					break;
//				++count;
//			}
//			return midRef(from, count);
//		}
//	}

//	bool same(const QString &lhs, const char *rhs) {
//		const int len = strlen(rhs);
//		if (lhs.length() != len)
//			return false;
//		for (int i=0; i<len; ++i) {
//			if (lhs.at(i).unicode() != rhs[i])
//				return false;
//		}
//		return true;
//	}
//	bool same(int at, ushort ucs) {return src.at(at).unicode() == ucs;}
//	static void appendAsLower(QString &dst, const QStringRef &src) {
//		dst.reserve(dst.size() + src.size());
//		for (int i=0; i<src.size(); ++i)
//			dst += src.at(i).toLower();
//	}
//	QChar qchar() const {return src.at(pos);}
//	ushort ucs() const {return src.at(pos).unicode();}
//	bool atEnd() const {return pos >= src.length();}
//	bool skipSpace() {for (; !atEnd() && isspace(src.at(pos).unicode()); ++pos) ; return atEnd();}
//	void parseTag() {
//		Q_ASSERT(ucs() == '<');
//		nodes.append(Node());
//		Node::Tag &tag = nodes.last().tag;
//		if (skipSpace())
//			return;
////#define CHECK_TAG_END (atEnd() || src.at(pos).unicode() == '>')
//		appendAsLower(tag.elem, parseWord());
//		if (same(tag.elem, "!--")) {
//			pos += 3;
//			while (!atEnd()) {
//				if (same(pos-2, '-') && same(pos-1, '-') && same(pos, '>')) {
//					++pos;
//					break;
//				}
//			}
//			return;
//		}
//		while (!atEnd()) {
//			if (skipSpace())
//				return;
//			if (ucs() == '>') {
//				++pos;
//				return;
//			}
//			tag.attr.append(Node::Tag::Attr());
//			Node::Tag::Attr &attr = tag.attr.last();
//			appendAsLower(attr.name, parseWord());
//			if (skipSpace())
//				return;
//			if (ucs() == '=') {
//				++pos;
//				if (skipSpace())
//					return;
//				attr.value = parseWord().toString();
//			}
//		}
//	}

//	int pos;
//	QStringRef src;
//	QList<Node> nodes;
//};


//RichText::RichText()
//: d(new Data) {
////	d->root = 0;
//}

//void RichText::setText(const QString &text) {
//	Parser p;
//	p.parse(text.midRef(0));
////	d->root = RootBlock::make(text, QFont(), QPen());
////	qDebug() << "html:" << d->root->toHtml();
//}

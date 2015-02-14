#ifndef RICHTEXTHELPER_HPP
#define RICHTEXTHELPER_HPP

class RichTextHelper {
public:
    static constexpr const auto QCI = Qt::CaseInsensitive;

    virtual ~RichTextHelper() {}

    SIA _Same(const QString &str, const char *latin1) -> bool
    { return !str.compare(_L(latin1), QCI); }

    SIA _Same(const QStringRef &str, const char *latin1) -> bool
    { return !str.compare(_L(latin1), QCI); }

    static auto toInt(const QStringRef &text) -> int;
    static auto toColor(const QStringRef &text) -> QColor;
    static auto isRightBracket(ushort c) -> bool {return c == '>';}
    static auto isSeparator(ushort c) -> bool
        { return _IsOneOf(c, ' ', '\t', '\r', '\n'); }
    static auto isWhitespace(ushort c) -> bool { return _IsOneOf(c, ' ', '\t'); }
    static auto isNewLine(ushort c) -> bool { return _IsOneOf(c, '\r', '\n'); }
    static auto replace(const QStringRef &str, const QString &from,
                        const QString &to, Qt::CaseSensitivity s = QCI) -> QString;
    static auto trim(const QStringRef &text) -> QStringRef
    {
        if (text.isEmpty()) return QStringRef();
        int start = 0, end = text.size();
        while (start < end && isSeparator(text.at(start).unicode())) ++start;
        while (end > start && isSeparator(text.at(end-1).unicode())) --end;
        return start < end ? text.mid(start, end-start) : QStringRef();
    }
    static auto skipSeparator(int &pos, const QStringRef &text) -> bool
    {
        for (; pos < text.size() && isSeparator(text.at(pos).unicode()); ++pos) ;
        return pos >= text.size(); // true for end
    }
    static auto skipSeparator(int &pos, const QString &text) -> bool
        { return skipSeparator(pos, text.midRef(0)); }

    static auto entityCharacter(const QStringRef &entity) -> QChar;
    static auto toFontPixelSize(const QStringRef &size) -> int;
    static auto pixelSizeToPointSize(double pt) -> int;

    struct Tag {
        struct Attr {
            Attr() { }
            Attr(const QStringRef &name, const QStringRef &value)
                : name(name), value(value) { }
            QStringRef name, value;
        };
        auto index(const char *attr) const -> int
        {
            for (int i = 0; i < this->attr.size(); ++i) {
                if (_Same(this->attr[i].name, attr))
                    return i;
            }
            return -1;
        }
        auto value(const char *attr) const -> QStringRef
        {
            const int idx = index(attr);
            return idx < 0 ? QStringRef() : this->attr[idx].value;
        }
        QMap<int, QVariant> style() const;
        int pos = -1;
        QStringRef name;
        QList<Attr> attr;
    };
    static auto parseTag(const QStringRef &text, int &pos) -> Tag;
    static auto parseTag(const QString &text, int &pos) -> Tag
        { return parseTag(text.midRef(0, -1), pos); }
    static auto innerText(const QString &open, const QString &close,
                          const QStringRef &text, QStringRef &block,
                          int &pos, Tag &tag) -> int;
    static auto innerText(const QString &open, const QString &close,
                          const QString &text, QStringRef &block,
                          int &pos, Tag &tag) -> int
        { return innerText(open, close, text.midRef(0), block, pos, tag); }
};

#endif // RICHTEXTHELPER_HPP

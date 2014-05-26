#ifndef RICHTEXTHELPER_HPP
#define RICHTEXTHELPER_HPP

#include "stdafx.hpp"

class RichTextHelper {
public:
    virtual ~RichTextHelper() {}

    static auto toInt(const QStringRef &text) -> int;
    static auto toColor(const QStringRef &text) -> QColor;
    static auto isRightBracket(ushort c) -> bool {return c == '>';}
    static auto isSeparator(ushort c) -> bool
        { return c == ' ' || c == '\t' || c == '\r' || c== '\n'; }
    static auto isWhitespace(ushort c) -> bool { return c == ' ' || c == '\t'; }
    static auto isNewLine(ushort c) -> bool { return c == '\r' || c == '\n'; }
    static auto replace(const QStringRef &str, const QLatin1String &from,
                        const QLatin1String &to,
                        Qt::CaseSensitivity s = Qt::CaseInsensitive) -> QString;
    static auto indexOf(const QStringRef &ref, QRegExp &rx, int from=0) -> int;
    static auto indexOf(const QStringRef &ref,
                        QRegularExpression &rx, int from = 0) -> int;
    static auto match(QRegularExpression &rx, const QStringRef &ref,
                      int from = 0) -> QRegularExpressionMatch
        { return rx.match(*ref.string(), from + ref.position()); }
    static auto trim(const QStringRef &text) -> QStringRef
    {
        if (text.isEmpty()) return QStringRef();
        int start = 0, end = text.size();
        while (start < end && isSeparator(text.at(start).unicode())) ++start;
        while (end > start && isSeparator(text.at(end-1).unicode())) --end;
        return start < end ? _MidRef(text, start, end-start) : QStringRef();
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
        {return parseTag(text.midRef(0, -1), pos);}
    static auto innerText(const char *open, const char *close,
                          const QStringRef &text, QStringRef &block,
                          int &pos, Tag &tag) -> int;
    static auto innerText(const char *open, const char *close,
                          const QString &text, QStringRef &block,
                          int &pos, Tag &tag) -> int
        { return innerText(open, close, text.midRef(0, -1), block, pos, tag); }
};

#endif // RICHTEXTHELPER_HPP

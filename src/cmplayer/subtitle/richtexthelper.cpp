#include "richtexthelper.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(RichText)

auto RichTextHelper::toInt(const QStringRef &text) -> int
{
    int ret = 0;
    for (int i=0; i<text.size(); ++i) {
        const ushort c = text.at(i).unicode();
        if (Q_LIKELY('0' <= c && c <= '9'))
            ret = ret*10 + (c-'0');
        else
            break;
    }
    return ret;
}

auto RichTextHelper::toColor(const QStringRef &text) -> QColor
{
    if (text.isEmpty())
        return QColor();
    int pos = 0;
    for (; pos < text.size(); ++pos) {
        if (text.at(pos).unicode() != '#')
            break;
    }
    if (pos > 0)
        return QColor(_L('#') % text.mid(pos));
    int i=0;
    for (; i<text.size() && _IsHexNumber(text.at(i).unicode()); ++i) ;
    if (i == text.size())
        return QColor(_L("#") % text);
    return QColor(text.toString());
}

auto RichTextHelper::replace(const QStringRef &str, const QLatin1String &from,
                             const QLatin1String &to,
                             Qt::CaseSensitivity s) -> QString
{
    QString text;
    int start = 0;
    const int len = strlen(from.latin1());
    for (;;) {
        const int pos = str.indexOf(from, start, s);
        if (pos < 0) {
            text += _MidRef(str, start);
            break;
        } else {
            text += _MidRef(str, start, pos - start);
            text += to;
            start = pos + len;
        }
    }
    return text;
}

auto RichTextHelper::pixelSizeToPointSize(double pt) -> int
{
    const double dpi = QApplication::desktop()->logicalDpiY();
    return pt*dpi/72.0 + 0.5;
}

auto RichTextHelper::toFontPixelSize(const QStringRef &size) -> int
{
    int px = 0, i = 0;
    while (i<size.size()) {
        const ushort c = size.at(i).unicode();
        if ('0' <= c && c <= '9') {
            px = px*10 + (c - '0');
        } else
            break;
        ++i;
    }
    if (i >= size.size())
        return px;
    const QStringRef unit = _MidRef(size, i);
    if (unit.size() == 2 && unit.compare("pt", Qt::CaseInsensitive) == 0)
        px = pixelSizeToPointSize(px);
    return px;
}

auto RichTextHelper::parseTag(const QStringRef &text,
                              int &pos) -> RichTextHelper::Tag
{
    auto at = [&text] (int idx) {return text.at(idx).unicode();};
    if (at(pos) != '<')
        return Tag();
    Tag tag;
    tag.pos = pos;
    ++pos;
    if (skipSeparator(pos, text))
        return Tag();
    if (at(pos) == '!') { // skip comment
        pos = text.indexOf(_L("!--"), pos);
        if (pos < 0) {
            pos = text.size();
            return Tag();
        }
        tag.name = _MidRef(text, pos, 3);
        pos = text.indexOf('>', pos);
        if (pos < 0)
            pos = text.size();
        return tag;
    }
    int start = pos;
    while (pos < text.size() && !isSeparator(at(pos)) && at(pos) != '>')
        ++pos;
    tag.name = _MidRef(text, start, pos - start);
    if (tag.name.startsWith('/')) {
        while (pos < text.size() && at(pos) != '>')
            ++pos;
        if (pos >= text.size())
            return Tag();
        ++pos;
        return tag;
    }
    if (skipSeparator(pos, text))
        return Tag();
    ushort q = 0;
    while (pos < text.size()) {
        if (skipSeparator(pos, text))
            return Tag();
        const ushort c = at(pos);
        if (c == '>') {
            ++pos;
            return tag;
        }
        start = pos;
        while (pos < text.size()) {
            const ushort c = at(pos);
            if (isSeparator(c) || c == '=' || c == '>')
                break;
            ++pos;
        }
        Tag::Attr attr;
        attr.name = _MidRef(text, start, pos - start);
        if (skipSeparator(pos, text))
            return Tag();
        if (at(pos) == '=') {
            // find value
            if (skipSeparator(++pos, text))
                return Tag();
            if (at(pos) == '\'') {
                q = '\'';
                ++pos;
            } else if (at(pos) ==  '"') {
                q = '"';
                ++pos;
            } else
                q = 0;
            start = pos;
            while (pos < text.size()) {
                const ushort c = at(pos);
                const bool q_end = (q && c == q && at(pos-1) != '\\');
                if (q_end || (!q && (isSeparator(c) || c == '>'))) {
                    attr.value = _MidRef(text, start, pos - start);
                    tag.attr.push_back(attr);
                    if (q_end)
                        ++pos;
                    break;
                }
                ++pos;
            }
        } else {
            tag.attr.push_back(attr);
        }
    }
    return Tag();
}

auto RichTextHelper::entityCharacter(const QStringRef &entity) -> QChar
{
    Q_UNUSED(entity);
#define RETURN(ent, c) {if (_Same(entity, ent)) return QChar(c);}
    RETURN("nbsp", ' ');
    RETURN("amp", '&');
    RETURN("lt", '<');
    RETURN("gt", '>');
#undef RETURN
    return QChar();
}

auto RichTextHelper::indexOf(const QStringRef &ref, QRegExp &rx,
                             int from) -> int
{
    const int pos = ref.position();
    const int idx = ref.string()->indexOf(rx, from + pos) - pos;
    return 0 <= idx && idx < ref.length() ? idx : -1;
}

auto RichTextHelper::indexOf(const QStringRef &ref, QRegularExpression &rx,
                             int from) -> int
{
    const int pos = ref.position();
    const int idx = ref.string()->indexOf(rx, from + pos) - pos;
    return 0 <= idx && idx < ref.length() ? idx : -1;
}

auto RichTextHelper::innerText(const char *open, const char *close,
                               const QStringRef &text, QStringRef &block,
                               int &pos, Tag &tag) -> int
{
    while (pos < text.size()) {
        const QChar c = text.at(pos);
        if (c.unicode() == '<') {
            tag = parseTag(text, pos);
            if (_Same(tag.name, open))
                break;
        } else
            ++pos;
    }
    if (pos >= text.size() || tag.name.isEmpty())
        return 0;
    int ret = 1;
    QRegExp rx(_L("<[\\s\\n\\r]*(") % _L(close)
               % _L(")(>|[^0-9a-zA-Z>]+[^>]*>)"));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    int start = pos;
    int end = indexOf(text, rx, start);
    if (end < 0) {
        end = pos = text.size();
    } else {
        pos = end;
        const QString cap = rx.cap(1);
        if (Q_UNLIKELY(cap.startsWith('/'))) {
            const QStringRef closer = cap.midRef(1, -1);
            if (_Same(closer, open))
                pos += rx.matchedLength();
        }
    }
    block = _MidRef(text, start, end - start);
    return ret;
}

QMap<int, QVariant> RichTextHelper::Tag::style() const {
    QMap<int, QVariant> style;
    if (_Same(name, "b"))
        style[QTextFormat::FontWeight] = QFont::Bold;
    else if (_Same(name, "u"))
        style[QTextFormat::FontUnderline] = true;
    else if (_Same(name, "i"))
        style[QTextFormat::FontItalic] = true;
    else if (_Same(name, "s") || _Same(name, "strike"))
        style[QTextFormat::FontStrikeOut] = true;
    else if (_Same(name, "sup"))
        style[QTextFormat::TextVerticalAlignment]
                = QTextCharFormat::AlignSuperScript;
    else if (_Same(name, "sub"))
        style[QTextFormat::TextVerticalAlignment]
                = QTextCharFormat::AlignSubScript;
    else if (_Same(name, "font")) {
        for (int i=0; i<attr.size(); ++i) {
            if (_Same(attr[i].name, "color")) {
                const auto color = toColor(trim(attr[i].value));
                if (color.isValid())
                    style[QTextFormat::ForegroundBrush] = QBrush(color);
                else
                    _Debug("%% is not a valid name for color",
                           trim(attr[i].value));
            } else if (_Same(attr[i].name, "face"))
                style[QTextFormat::FontFamily]
                        = trim(attr[i].value).toString();
            else if (_Same(attr[i].name, "size"))
                style[QTextFormat::FontPixelSize]
                        = toFontPixelSize(trim(attr[i].value));
        }
    }
    return style;
}

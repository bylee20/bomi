#include "richtexthelper.hpp"
#include "misc/log.hpp"
#include <QDesktopWidget>

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

SIA _IsNumber(ushort c) -> bool
{return _InRange<ushort>('0', c, '9');}

SIA _IsHexNumber(ushort c) -> bool
{
    return _IsNumber(c) || _InRange<ushort>('a', c, 'f')
                        || _InRange<ushort>('A', c, 'F');
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
        return QColor('#'_q % text.mid(pos));
    int i=0;
    for (; i<text.size() && _IsHexNumber(text.at(i).unicode()); ++i) ;
    if (i == text.size())
        return QColor(u"#"_q % text);
    return QColor(text.toString());
}

auto RichTextHelper::replace(const QStringRef &str, const QString &from,
                             const QString &to, Qt::CaseSensitivity s) -> QString
{
    QString text;
    int start = 0;
    const int len = from.size();
    for (;;) {
        const int pos = str.indexOf(from, start, s);
        if (pos < 0) {
            text += str.mid(start);
            break;
        } else {
            text += str.mid(start, pos - start);
            text += to;
            start = pos + len;
        }
    }
    return text;
}

auto RichTextHelper::pixelSizeToPointSize(double pt) -> int
{
    const double dpi = qApp->desktop()->logicalDpiY();
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
    const QStringRef unit = size.mid(i);
    if (unit.size() == 2 && unit.compare("pt"_a, QCI) == 0)
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
        pos = text.indexOf("!--"_a, pos);
        if (pos < 0) {
            pos = text.size();
            return Tag();
        }
        tag.name = text.mid(pos, 3);
        pos = text.indexOf('>'_q, pos);
        if (pos < 0)
            pos = text.size();
        return tag;
    }
    int start = pos;
    while (pos < text.size() && !isSeparator(at(pos)) && at(pos) != '>')
        ++pos;
    tag.name = text.mid(start, pos - start);
    if (tag.name.startsWith('/'_q)) {
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
            if (isSeparator(c) || _IsOneOf(c, '=', '>'))
                break;
            ++pos;
        }
        Tag::Attr attr;
        attr.name = text.mid(start, pos - start);
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
                    attr.value = text.mid(start, pos - start);
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
#define RETURN(ent, c) {if (!entity.compare(ent##_a, QCI)) return c##_q;}
    RETURN("nbsp", ' ');
    RETURN("amp", '&');
    RETURN("lt", '<');
    RETURN("gt", '>');
#undef RETURN
    return QChar();
}

auto RichTextHelper::innerText(const QString &open, const QString &close,
                               const QStringRef &text, QStringRef &block,
                               int &pos, Tag &tag) -> int
{
    while (pos < text.size()) {
        const QChar c = text.at(pos);
        if (c.unicode() == '<') {
            tag = parseTag(text, pos);
            if (!tag.name.compare(open, QCI))
                break;
        } else
            ++pos;
    }
    if (pos >= text.size() || tag.name.isEmpty())
        return 0;
    int ret = 1;
    QRegEx rx(R"(<[\s\n\r]*()"_a % close % ")(>|[^0-9a-zA-Z>]+[^>]*>)"_a,
              QRegEx::CaseInsensitiveOption);
    int start = pos;
    const auto match = rx.match(text.toString(), start);
    int end = match.capturedStart();
    if (end < 0) {
        end = pos = text.size();
    } else {
        pos = end;
        const auto cap = match.capturedRef(1);
        if (Q_UNLIKELY(cap.startsWith('/'_q))) {
            if (!cap.mid(1).compare(open, QCI))
                pos += match.capturedLength();
        }
    }
    block = text.mid(start, end - start);
    return ret;
}

auto RichTextHelper::Tag::style() const -> QMap<int, QVariant>
{
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

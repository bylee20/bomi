#include "subtitle.hpp"
#include "subtitle_parser.hpp"
#include "global.hpp"
#include "misc/charsetdetector.hpp"

auto SubComp::name() const -> QString
{
    return m_klass.isEmpty() ? m_file : m_file % _L("(") % m_klass % _L(")");
}

auto Subtitle::component(double frameRate) const -> SubComp
{
    if (m_comp.isEmpty())
        return SubComp();
    SubComp comp;
    for (int i=0; i<m_comp.size(); ++i)
        comp.unite(m_comp[i], frameRate);
    return comp;
}

SubComp::SubComp() {
    m_capts[0].index = 0;
}

SubComp::SubComp(const QFileInfo &file, const QString &enc, int id, SyncType base)
: m_file(file.fileName()), m_info(file.absoluteFilePath(), enc), m_base(base), m_id(id) {
    m_capts[0].index = 0;
}

auto SubComp::united(const SubComp &other, double frameRate) const -> SubComp
{
    return SubComp(*this).unite(other, frameRate);
}

auto SubComp::start(int time, double frameRate) const -> SubComp::const_iterator
{
    if (isEmpty() || time < 0)
        return end();
    return --finish(time, frameRate);
}

auto SubComp::finish(int time, double frameRate) const -> SubComp::const_iterator
{
    if (isEmpty() || time < 0)
        return end();
    int key = time;
    if (m_base == Frame) {
        if (frameRate < 0.0)
            return end();
        key = qRound(time*0.001*frameRate);
    }
    return upperBound(key);
}

auto SubComp::unite(const SubComp &rhs, double fps) -> SubComp&
{
    if (this == &rhs || rhs.isEmpty())
        return *this;
    else if (isEmpty())
        return *this = rhs;
    auto convertKeyBase = [this] (int key, SyncType from, SyncType to, double frameRate) {
        return  (from == to) ? key : ((to == Time) ? msec(key, frameRate) : frame(key, frameRate));
    };

    auto it1 = m_capts.begin();
    auto it2 = rhs.begin();
    const auto k2 = convertKeyBase(it2.key(), rhs.base(), m_base, fps);
    if (it2.key() < it1.key()) {
        while (k2 < it1.key()) {
            m_capts.insert(k2, *it2);
            ++it2;
        }
    } else if (k2 == it1.key()){
        *it1 += *it2;
        ++it2;
    } else
        it1 = --m_capts.lowerBound(k2);

    while (it2 != rhs.end()) {
        auto &cap1 = *it1;
//        const int ka = it1.key();
        const int kb = ++it1 != m_capts.end() ? it1.key() : -1;
//        Q_ASSERT(ka < k2);
        int k2 = 0;
        while (it2 != rhs.end()) {
            k2 = convertKeyBase(it2.key(), rhs.base(), m_base, fps);
            if (!(kb == -1 || k2 < kb))
                break;
            *m_capts.insert(k2, cap1) += *it2;
            ++it2;
        }
        if (it2 == rhs.end())
            break;
        if (k2 == kb)
            *it1 += *it2++;
        else if (it2 != rhs.begin())
            *it1 += *(it2-1);
    }

    auto it = m_capts.begin();
    for (int idx = 0; it != m_capts.end(); ++idx, ++it)
        it->index = idx;
    return *this;
}

auto Subtitle::caption(int time, double fps) const -> RichTextDocument
{
    if (m_comp.isEmpty())
        return RichTextDocument();
    RichTextDocument caption;
    for (int i=0; i<m_comp.size(); ++i) {
        const auto it = m_comp[i].start(time, fps);
        if (it != m_comp[i].end())
            caption += *it;
    }
    return caption;
}

auto Subtitle::load(const QString &file, const QString &enc, double accuracy) -> bool
{
    QString encoding;
    if (accuracy > 0.0)
        encoding = CharsetDetector::detect(file, accuracy);
    if (encoding.isEmpty())
        encoding = enc;
    *this = parse(file, encoding);
    return !isEmpty();
}

auto Subtitle::parse(const QString &file, const QString &enc) -> Subtitle
{
    return SubtitleParser::parse(file, enc);
}

auto Subtitle::isEmpty() const -> bool
{
    if (m_comp.isEmpty())
        return true;
    for (int i=0; i<m_comp.size(); ++i) {
        if (!m_comp[i].isEmpty())
            return false;
    }
    return true;
}

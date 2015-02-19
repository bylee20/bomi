#ifndef ENCODINGINFO_HPP
#define ENCODINGINFO_HPP

class EncodingInfo {
public:
    EncodingInfo();
    ~EncodingInfo();
    auto name() const -> QString { return m_name; }
    auto group() const -> QString { return m_group; }
    auto subgroup() const -> QString { return m_subgroup; }
    auto description() const -> QString;
    auto codec() const -> QTextCodec*;
    static auto all() -> QVector<EncodingInfo>;
    static auto categorized() -> QVector<QVector<EncodingInfo>>;
private:
    EncodingInfo(const QString &name, const QString &group,
                 const QString &sub = QString());
    QString m_name, m_group, m_subgroup;
};

#endif // ENCODINGINFO_HPP

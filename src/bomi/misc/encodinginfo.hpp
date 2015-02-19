#ifndef ENCODINGINFO_HPP
#define ENCODINGINFO_HPP

class EncodingInfo {
public:
    EncodingInfo();
    ~EncodingInfo();
    auto operator == (const EncodingInfo &rhs) const -> bool
        { return m_mib == rhs.m_mib; }
    auto operator != (const EncodingInfo &rhs) const -> bool
        { return !operator == (rhs); }
    auto mib() const -> int { return m_mib; }
    auto isValid() const -> bool { return m_mib > 0; }
    auto name() const -> QString { return m_name; }
    auto group() const -> QString { return m_group; }
    auto subgroup() const -> QString { return m_subgroup; }
    auto description() const -> QString;
    auto codec() const -> QTextCodec*;
    static auto all() -> const QVector<EncodingInfo>&;
    static auto categorized() -> QVector<QVector<EncodingInfo>>;
    auto toJson() const -> QJsonValue;
    auto setFromJson(const QJsonValue &json) -> bool;
    static auto fromMib(int mib) -> EncodingInfo;
    static auto fromName(const QString &name) -> EncodingInfo;
    static auto fromCodec(const QTextCodec *codec) -> EncodingInfo;
private:
    EncodingInfo(int mib, const QString &name,
                 const QString &group, const QString &sub = QString());
    int m_mib = 0;
    QString m_name, m_group, m_subgroup;
};

Q_DECLARE_METATYPE(EncodingInfo)

#endif // ENCODINGINFO_HPP

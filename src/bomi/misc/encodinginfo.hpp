#ifndef ENCODINGINFO_HPP
#define ENCODINGINFO_HPP

class EncodingInfo {
public:
    enum Category { General, Subtitle, Playlist, CategoryMax };

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
    static auto grouped() -> QVector<QVector<EncodingInfo>>;
    auto toJson() const -> QJsonValue;
    auto setFromJson(const QJsonValue &json) -> bool;
    static auto detect(Category c, const QByteArray &data) -> EncodingInfo;
    static auto detect(Category c, const QString &file, int length =  1024*500) -> EncodingInfo;
    static auto detect(Category c, const EncodingInfo &fb, const QByteArray &data) -> EncodingInfo;
    static auto detect(Category c, const EncodingInfo &fb, const QString &file, int length =  1024*500) -> EncodingInfo;
    static auto utf8() -> EncodingInfo { return EncodingInfo::fromMib(106); }
    static auto fromMib(int mib) -> EncodingInfo;
    static auto fromName(const QString &name) -> EncodingInfo;
    static auto fromCodec(const QTextCodec *codec) -> EncodingInfo;
    static auto default_(Category c = General) -> EncodingInfo { return _default(c); }
    static auto setDefault(Category c, const EncodingInfo &def, double autodetect = -1) -> void;
private:
    static auto _default(Category c) -> EncodingInfo&;
    static auto _confidence(Category c) -> double&;
    EncodingInfo(int mib, const QString &name,
                 const QString &group, const QString &sub = QString());
    int m_mib = 0;
    QString m_name, m_group, m_subgroup;
};

Q_DECLARE_METATYPE(EncodingInfo)

auto operator << (QDataStream &out, const EncodingInfo &e) -> QDataStream&;
auto operator >> (QDataStream &in, EncodingInfo &e) -> QDataStream&;

#endif // ENCODINGINFO_HPP

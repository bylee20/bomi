#ifndef YOUTUBEDL_HPP
#define YOUTUBEDL_HPP

#include <QObject>
#include "player/playlist.hpp"

struct YouTubeFormat {
    auto id() const -> QString { return m_id; }
    auto extension() const -> QString { return m_ext; }
    auto height() const -> int { return m_height; }
    auto width() const -> int { return m_width; }
    auto fps() const -> double { return m_fps; }
    auto preference() const -> int { return m_prefer; }
    auto isDash() const -> bool { return m_dash; }
    auto isAudio() const -> bool { return m_audio; }
    auto tbr() const -> int { return m_tbr; }
    auto bps() const -> int { return m_bps; }
    auto bitrate() const -> int { return m_bps ? m_bps : m_tbr; }
    auto codec() const -> QString { return m_codec; }
    auto is3D() const -> bool { return m_3d; }
    auto url() const -> QString { return m_url; }
    auto isValid() const -> bool;
    static auto fromJson(const QJsonObject &json) -> YouTubeFormat;
private:
    QString m_ext, m_url, m_id, m_codec;
    bool m_dash = false, m_audio = false, m_3d = false;
    int m_height = 0, m_width = 0, m_prefer = 0, m_tbr = 0, m_bps = 0;
    double m_fps = 0.0;
};

class YouTubeDL : public QObject
{
    using GetFormatFunc = std::function<void(QList<YouTubeFormat>*,const QString&)>;
public:
    enum Error {
        NoError, Canceled, FailedToStart, Timeout, Crashed,
        ReadError, UnknownError, Unsupported
    };
    struct Result {
        bool direct = false;
        int duration = -1;
        Playlist playlist;
        QString url, title, audio;
    };

    YouTubeDL(QObject *parent = nullptr);
    ~YouTubeDL();
    static auto supports(const QString &url) -> bool;
    auto userAgent() const -> QString;
    auto setUserAgent(const QString &ua) -> void;
    auto setProgram(const QString &program) -> void;
    auto program() const -> QString;
    auto setTimeout(int timeout) -> void;
    auto timeout() const -> int;
    auto cookies() const -> QString;
    auto run(const QString &url) -> bool;
    auto error() const -> Error;
    auto result() const -> Result;
    auto cancel() -> void;
    auto setGetFormat(GetFormatFunc &&func) -> void;
    auto setAskVideoQuality(bool ask) -> void;
private:
    struct Data;
    Data *d;
};

class YouTubeDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(YouTubeDialog)
public:
    YouTubeDialog();
    ~YouTubeDialog();
    auto setFormats(const QList<YouTubeFormat> &formats, const QString &def) -> void;
    auto formats() const -> QList<YouTubeFormat>;
private:
    struct Data;
    Data *d;
};

#endif // YOUTUBEDL_HPP

#ifndef YOUTUBEDL_HPP
#define YOUTUBEDL_HPP

#include <QObject>
#include "player/playlist.hpp"

struct YouTubeFormat {
    auto operator < (const YouTubeFormat &rhs) const -> bool;
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
    auto isLive() const -> bool;
    auto description() const -> QString;
    static auto fromJson(const QJsonObject &json) -> YouTubeFormat;
private:
    QString m_ext, m_url, m_id, m_codec;
    bool m_dash = false, m_audio = false, m_3d = false, m_live = false;
    int m_height = 0, m_width = 0, m_prefer = 0, m_tbr = 0, m_bps = 0;
    double m_fps = 0.0;
};

class YouTubeDL : public QObject {
public:
    enum Error {
        NoError, Canceled, FailedToStart, Timeout, Crashed,
        ReadError, UnknownError, Unsupported
    };
    struct Result {
        bool direct = false, live = false;
        int duration = -1;
        Playlist playlist;
        QString url, title, mrl, selection;
        YouTubeFormat audio;
        QList<YouTubeFormat> videos;
        auto clear() -> void { *this = Result(); }
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
    auto setPreferredFormat(int height, int fps, const QString &container) -> void;
    auto select(const QList<YouTubeFormat> &formats) const -> int;
private:
    struct Data;
    Data *d;
};

#endif // YOUTUBEDL_HPP

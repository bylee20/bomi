#ifndef MPRIS_HPP
#define MPRIS_HPP

#include "stdafx.hpp"

namespace mpris {

class MediaPlayer2 : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool Fullscreen READ isFullScreen WRITE setFullScreen)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullScreen)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
public:
    MediaPlayer2(QObject *parent);
    ~MediaPlayer2();
    bool canQuit() const { return true; }
    bool isFullScreen() const;
    void setFullScreen(bool fs);
    bool canSetFullScreen() const { return true; }
    bool canRaise() const { return true; }
    bool hasTrackList() const { return false; }
    QString identity() const { return QLatin1String("CMPlayer"); }
    QString desktopEntry() const { return QLatin1String("cmplayer"); }
    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;
public slots:
    void Raise();
    void Quit();
private:
    struct Data;
    Data *d;
};

class Player : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ isShuffled WRITE setShuffled)
    Q_PROPERTY(QVariantMap Metadata READ metaData)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qint64 Position READ time)
    Q_PROPERTY(double MinimumRate READ minRate CONSTANT FINAL)
    Q_PROPERTY(double MaximumRate READ maxRate CONSTANT FINAL)
    Q_PROPERTY(bool CanGoNext READ hasNext)
    Q_PROPERTY(bool CanGoPrevious READ hasPrevious)
    Q_PROPERTY(bool CanPlay READ isPlayable)
    Q_PROPERTY(bool CanPause READ isPausable)
    Q_PROPERTY(bool CanSeek READ isSeekable)
    Q_PROPERTY(bool CanControl READ isControllable)
public:
    Player(QObject *parent);
    ~Player();
    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &status);
    double rate() const;
    void setRate(double rate);
    bool isShuffled() const;
    void setShuffled(bool s);
    QVariantMap metaData() const;
    double volume() const;
    void setVolume(double volume);
    qint64 time() const;
    double minRate() const { return 0.1; }
    double maxRate() const { return 10.0; }
    bool hasNext() const;
    bool hasPrevious() const;
    bool isPlayable() const;
    bool isPausable() const;
    bool isSeekable() const;
    bool isControllable() const { return true; }
public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qint64 offset);
    void SetPosition(const QDBusObjectPath &track, qint64 position);
    void OpenUri(const QString &url);
signals:
    void Seeked(qint64 Position);
private:
    struct Data;
    Data *d;
};

class RootObject : public QObject {
    Q_OBJECT
public:
    RootObject(QObject *parent = nullptr);
    ~RootObject();
private:
    MediaPlayer2 *m_mp2 = nullptr;
    Player *m_player = nullptr;
    QString m_name;

};

}

#endif // MPRIS_HPP

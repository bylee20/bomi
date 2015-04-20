#ifndef MPRIS_HPP
#define MPRIS_HPP

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
    auto canQuit() const -> bool { return true; }
    auto isFullScreen() const -> bool;
    auto setFullScreen(bool fs) -> void;
    auto canSetFullScreen() const -> bool { return true; }
    auto canRaise() const -> bool { return true; }
    auto hasTrackList() const -> bool { return false; }
    auto identity() const -> QString { return QLatin1String("bomi"); }
    auto desktopEntry() const -> QString { return QLatin1String("bomi"); }
    auto supportedUriSchemes() const -> QStringList;
    auto supportedMimeTypes() const -> QStringList;
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
    auto playbackStatus() const -> QString;
    auto loopStatus() const -> QString;
    auto setLoopStatus(const QString &status) -> void;
    auto rate() const -> double;
    auto setRate(double rate) -> void;
    auto isShuffled() const -> bool;
    auto setShuffled(bool s) -> void;
    auto metaData() const -> QVariantMap;
    auto volume() const -> double;
    auto setVolume(double volume) -> void;
    auto time() const -> qint64;
    auto minRate() const -> double { return 0.1; }
    auto maxRate() const -> double { return 10.0; }
    auto hasNext() const -> bool;
    auto hasPrevious() const -> bool;
    auto isPlayable() const -> bool;
    auto isPausable() const -> bool;
    auto isSeekable() const -> bool;
    auto isControllable() const -> bool { return true; }
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
    auto updateMetaData() -> void;
    auto customEvent(QEvent *ev) -> void final;
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

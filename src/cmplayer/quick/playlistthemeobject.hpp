#ifndef PLAYLISTTHEMEOBJECT_HPP
#define PLAYLISTTHEMEOBJECT_HPP

#include <QObject>

struct PlaylistTheme {
    bool showLocation = true;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(PlaylistTheme);

class PlaylistThemeObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool showLocation READ showLocation NOTIFY changed)
public:
    auto set(const PlaylistTheme &theme) { m_theme = theme; emit changed(); }
    auto showLocation() const { return m_theme.showLocation; }
signals:
    void changed();
private:
    PlaylistTheme m_theme;
};

class PlaylistThemeWidget : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(PlaylistTheme value READ value WRITE setValue)
public:
    PlaylistThemeWidget(QWidget *parent): QCheckBox(parent) { }
    auto value() const -> PlaylistTheme
    {
        PlaylistTheme theme;
        theme.showLocation = isChecked();
        return theme;
    }
    auto setValue(const PlaylistTheme &theme)
    {
        setChecked(theme.showLocation);
    }
};

#endif // PLAYLISTTHEMEOBJECT_HPP

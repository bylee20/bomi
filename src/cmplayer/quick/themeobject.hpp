#ifndef THEMEOBJECT_HPP
#define THEMEOBJECT_HPP

struct OsdTheme;                        class OsdThemeObject;

class ThemeObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(OsdThemeObject *osd READ osd NOTIFY osdChanged)
public:
    ThemeObject(QObject *parent = nullptr);
    auto osd() const -> OsdThemeObject* { return m_osd; }
    auto setOsd(const OsdTheme &osd) -> void;
signals:
    void osdChanged();
private:
    OsdThemeObject *m_osd = nullptr;
};

#endif // THEMEOBJECT_HPP

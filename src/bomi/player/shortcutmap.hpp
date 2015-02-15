#ifndef SHORTCUTMAP_HPP
#define SHORTCUTMAP_HPP

class Shortcut {
    using Key = QKeySequence;
public:
    Shortcut() = default;
    Shortcut(const Shortcut &other)
        : m_id(other.m_id), m_default(other.m_default)
    { if (!m_default) m_keys = other.m_keys; }
    Shortcut(Shortcut &&other)
        : m_id(std::move(other.m_id)), m_default(other.m_default)
    { if (!m_default) m_keys.swap(other.m_keys); }

    auto operator = (const Shortcut &rhs) -> Shortcut&
    {
        if (this != &rhs) {
            m_id = rhs.m_id;
            if ((m_default = rhs.m_default))
                m_keys.clear();
            else
                m_keys = rhs.m_keys;
        }
        return *this;
    }
    auto operator = (Shortcut &&rhs) -> Shortcut&
    {
        m_id.swap(rhs.m_id);
        if ((m_default = rhs.m_default))
            m_keys.clear();
        else
            m_keys.swap(rhs.m_keys);
        return *this;
    }
    auto operator == (const Shortcut &rhs) const -> bool
    {
        if (m_id != rhs.m_id || m_default != rhs.m_default)
            return false;
        return m_default || m_keys == rhs.m_keys;
    }
    auto operator != (const Shortcut &rhs) const -> bool
        { return !operator == (rhs); }
    auto isDefault() const -> bool { return m_default;}
    auto keys() const -> QList<Key>;
    auto key(int i) const -> Key { return keys().value(i); }
    auto contains(const Key &key) const -> bool { return keys().contains(key); }
    auto setKeys(const QList<Key> &key) -> void
        { m_keys = key; m_default = false; }
    auto id() const -> QString { return m_id; }
    auto reset() -> void { m_default = true; m_keys.clear(); }
    auto clear() -> void { m_default = false; m_keys.clear(); }
private:
    friend class ShortcutMap;
    QString m_id;
    bool m_default = true;
    QList<QKeySequence> m_keys;
};

Q_DECLARE_METATYPE(Shortcut)

class ShortcutMap
{
    using Map = QMap<QString, Shortcut>;
public:
    using Key = QKeySequence;
    class const_iterator {
    public:
        const_iterator() = default;
        auto operator ++ () -> const_iterator& { ++m_it; return *this; }
        auto operator -- () -> const_iterator& { --m_it; return *this; }
        auto id() const -> QString { return m_it.key(); }
        auto keys() const -> QList<Key>;
    private:
        const_iterator(Map::const_iterator &&it): m_it(std::move(it)) { }
        friend class ShortcutMap;
        Map::const_iterator m_it;
    };
    enum Preset { Default, Movist };
    ShortcutMap();
    ShortcutMap(const ShortcutMap &other);
    ShortcutMap(ShortcutMap &&other);
    ~ShortcutMap();
    auto operator == (const ShortcutMap &rhs) const -> bool;
    auto operator != (const ShortcutMap &rhs) const -> bool
        { return !operator == (rhs); }
    auto operator = (const ShortcutMap &rhs) -> ShortcutMap&;
    auto operator = (ShortcutMap &&rhs) -> ShortcutMap&;
    auto reset(const QString &id) -> void;
    auto keys(const QString &id) const -> QList<Key>;
    auto clear(const QString &id) -> void;
    auto shortcut(const QString &id) const -> Shortcut;
    auto insert(const Shortcut &s) -> void;
    auto import(const QString &id, const QList<Key> &keys) -> void;
    static auto default_(const QString &id) -> QList<Key>;
    static auto preset(Preset p) -> ShortcutMap;
    auto validate() -> bool;
    auto begin() const -> const_iterator;
    auto end() const -> const_iterator;
    auto find(const QString &id) const -> const_iterator;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
private:
    struct Data;
    QSharedDataPointer<Data> d;
};

Q_DECLARE_METATYPE(ShortcutMap)

inline auto Shortcut::keys() const -> QList<Key>
{ return m_default ? ShortcutMap::default_(m_id) : m_keys; }

#endif // SHORTCUTMAP_HPP

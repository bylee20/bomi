#ifndef THEMEOBJECT_HELPER_HPP
#define THEMEOBJECT_HELPER_HPP

#define THEME_C(type, name) \
private: \
    type m_##name; \
    Q_PROPERTY(type *name READ name CONSTANT FINAL) \
public: \
    auto name() -> type* { return &m_##name; } \
private:

#define THEME_P(type, name) \
private: \
    Q_PROPERTY(type name READ name CONSTANT FINAL) \
public: \
    auto name() const -> type { return m.name; } \
private:

#define THEME_PV(type, name, var) \
private: \
    Q_PROPERTY(type name READ name CONSTANT FINAL) \
public: \
    auto name() const -> type { return var; } \
private:

#endif // THEMEOBJECT_HELPER_HPP


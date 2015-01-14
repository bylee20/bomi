#ifndef THEMEOBJECT_HELPER_HPP
#define THEMEOBJECT_HELPER_HPP

#define THEME_P(type, name) \
private: \
    type m_##name; \
    Q_PROPERTY(type *name READ name CONSTANT FINAL) \
public: \
    auto name() -> type* { return &m_##name; } \
private:

#endif // THEMEOBJECT_HELPER_HPP


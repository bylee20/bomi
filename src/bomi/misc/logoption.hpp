#ifndef LOGOPTION_HPP
#define LOGOPTION_HPP

#include "log.hpp"
#include "enum/logoutput.hpp"

SIA qHash(LogOutput key, uint seed) { return qHash(int(key), seed); }

struct LogOption {
    auto operator == (const LogOption &rhs) const -> bool;
    auto operator != (const LogOption &rhs) const -> bool
        { return !operator == (rhs); }
    auto level(LogOutput output) const -> Log::Level
        { return m_levels.value(output, Log::Off); }
    auto setLevel(LogOutput output, Log::Level lv) -> void
        { m_levels[output] = lv; }
    auto lines() const -> int { return m_lines; }
    auto setLines(int lines) -> void { m_lines = lines; }
    auto file() const -> QString { return m_file; }
    auto setFile(const QString &file) { m_file = file; }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto merge(const LogOption &other) -> void;
    static auto default_() -> LogOption;
private:
    QHash<LogOutput, Log::Level> m_levels;
    QString m_file;
    int m_lines = 0;
};

Q_DECLARE_METATYPE(LogOption)

class LogOptionWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(LogOption value READ option WRITE setOption NOTIFY optionChanged)
public:
    LogOptionWidget(QWidget *parent = nullptr);
    ~LogOptionWidget();
    auto option() const -> LogOption;
    auto setOption(const LogOption &option) -> void;
signals:
    void optionChanged();
private:
    struct Data;
    Data *d;
};

#endif // LOGOPTION_HPP

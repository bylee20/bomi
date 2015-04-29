#ifndef STEPINFO_HPP
#define STEPINFO_HPP

#include "enum/changevalue.hpp"
#include "tmp/type_traits.hpp"

struct StepInfo;

struct StepValue {
    StepValue();
    auto operator == (const StepValue &rhs) const -> bool
        { return toFixed(m_step) == toFixed(rhs.m_step) && m_info == rhs.m_info; }
    auto operator != (const StepValue &rhs) const -> bool
        { return !operator == (rhs); }

    auto set(double step) -> void;
    auto get() const -> double { return m_step; }

    auto changed(int prop, ChangeValue di) const -> int;
    auto changed(double prop, ChangeValue di) const -> double;
    auto text(ChangeValue direction) const -> QString;
    auto text(double prop) const -> QString;
    auto info() const -> const StepInfo* { return m_info; }
private:
    friend class Steps;
    StepValue(double v, const StepInfo *info): m_step(v), m_info(info) { }
    static constexpr qint64 Dec = 10000000000;
    static auto toFixed(double v) -> qint64 { return std::llround(v * Dec); }
    static auto fromFixed(qint64 v) -> double { return v / (double)Dec; }
    double m_step = 1;
    const StepInfo *m_info;
};

class Steps {
public:
    Steps();
    auto operator == (const Steps &rhs) const -> bool;
    auto operator != (const Steps &rhs) const -> bool
        { return !operator == (rhs); }
    StepValue seek1_sec, seek2_sec, seek3_sec, speed_pct;
    StepValue aspect_ratio, color_pct, zoom_pct, video_offset_pct;
    StepValue volume_pct, amp_pct, audio_sync_sec;
    StepValue sub_sync_sec, sub_pos_pct, sub_scale_pct;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(Steps)

struct StepInfo {
    StepInfo() { }
    StepInfo(const QByteArray &id, StepValue (Steps::*mem),
             double max, double single, int precision,
             const char *suffix, bool sign, double mul);
    const QByteArray id;
    const double max = 100, single = 1, multiply = 1;
    const int precision = 0;
    const bool sign = false;
    auto text(double step, double prop) const -> QString;
    auto text(double step, ChangeValue direction, bool sign) const -> QString;
    auto suffix() const -> QString { return qApp->translate("PrefDialog", m_suffix); }
    auto value(Steps *s) const -> StepValue& { return (s->*m_mem); }
    auto value(const Steps *s) const -> const StepValue& { return (s->*m_mem); }
private:
    friend class Steps;
    const char *const m_suffix = "";
    StepValue (Steps::*m_mem) = nullptr;
};

inline auto StepValue::changed(int prop, ChangeValue di) const -> int
{
    return prop + qRound(_EnumData(di) * m_step * info()->multiply);
}

inline auto StepValue::changed(double prop, ChangeValue di) const -> double
{
    return fromFixed(toFixed(prop) + toFixed(_EnumData(di) * m_step * info()->multiply));
}

#endif // STEPINFO_HPP

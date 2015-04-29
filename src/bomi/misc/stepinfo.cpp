#include "stepinfo.hpp"

static QList<const StepInfo*> s_info;

StepInfo::StepInfo(const QByteArray &id, StepValue (Steps::*mem),
                     double max, double single, int precision,
                     const char *suffix, bool sign, double mul)
    : id(id), max(max), single(single), multiply(mul)
    , precision(precision), sign(sign), m_suffix(suffix), m_mem(mem)
{
    s_info.push_back(this);
}

StepValue::StepValue()
{
    static const StepInfo dummy;
    m_info = &dummy;
}

auto StepValue::set(double step) -> void
{
    if (_InRange(0.0, step, m_info->max))
        m_step = step;
}

auto StepValue::text(ChangeValue direction) const -> QString
{
    return m_info->text(m_step, direction, true);
}

auto StepValue::text(double prop) const -> QString
{
    return m_info->text(m_step, prop);
}

Steps::Steps() {

#define STEP(name, val, ...) { \
    static const StepInfo info(#name, &Steps::name, __VA_ARGS__);\
    name = {val, &info}; }
    const auto sec = QT_TRANSLATE_NOOP("PrefDialog", " sec");
    const auto pct = QT_TRANSLATE_NOOP("PrefDialog", " %");
#define TIME(name, tm, sng) STEP(name, tm, 999, sng, 3, sec, true, 1000)

    TIME(seek1_sec,  5, 1);
    TIME(seek2_sec, 30, 1);
    TIME(seek3_sec, 60, 1);
    STEP(speed_pct, 10, 99, 1, 0, pct, false, 1e-2);

    STEP(aspect_ratio, 0.001, 9.99999, 0.001, 5, "", false, 1);
    STEP(color_pct, 1, 99, 1, 0, pct, true, 1);
    STEP(zoom_pct, 0.1, 99.999, 0.1, 3, pct, false, 1e-2);
    STEP(video_offset_pct, 0.1, 99.999, 0.1, 3, pct, true, 1e-2);

    STEP(volume_pct, 2, 99, 1, 0, pct, false, 1e-2);
    STEP(amp_pct, 10, 99, 1, 0, pct, false, 1e-2);
    TIME(audio_sync_sec, 0.2, 0.1);

    TIME(sub_sync_sec, 0.2, 0.1);
    STEP(sub_pos_pct, 1, 99, 1, 0, pct, false, 1e-2);
    STEP(sub_scale_pct, 1.0, 99.9, 1.0, 1, pct, true, 1e-2);

#undef STEP
#undef TIME
}

auto Steps::operator == (const Steps &rhs) const -> bool
{
    for (auto info : s_info) {
        if (info->value(this) != info->value(&rhs))
            return false;
    }
    return true;
}

auto Steps::toJson() const -> QJsonObject
{
    QJsonObject json;
    for (auto info : s_info)
        json.insert(_L(info->id), info->value(this).get());
    return json;
}

auto Steps::setFromJson(const QJsonObject &json) -> bool
{
    for (auto info : s_info) {
        auto it = json.find(_L(info->id));
        if (it == json.end())
            continue;
        const auto step = it.value().toDouble();
        if (!_InRange(0.0, step, info->max))
            continue;
        info->value(this).set(step);
    }
    sub_scale_pct.set(1);
    return true;
}

SIA decimals(double step) -> int
{
    if (step == 0)
        return 0;
    const auto exp = std::log10(qAbs(step + step*1e-10));
    if (exp >= 0)
        return 0;
    return 1 + (int)-exp;
}

//auto StepInfo::text(double prop) const -> QString
//{
////    qDebug() << m_suffix << suffix();
//    const auto info = this->info();
//    qDebug() << m_id << info.suffix();
//    return _NS(prop/info.mul, info.sign, info.prec) % info.suffix();
//}

//auto StepInfo::text(ChangeValue direction) const -> QString
//{
//    const auto info = this->info();
//    return _NS(_EnumData(direction) * m_step, true, info.prec) % info.suffix();
//}

auto StepInfo::text(double step, double prop) const -> QString
{
    return _NS(prop/multiply, sign, decimals(step)) % suffix();
}

auto StepInfo::text(double step, ChangeValue direction, bool sign) const -> QString
{
    return _NS(_EnumData(direction) * step, sign, decimals(step)) % suffix();
}

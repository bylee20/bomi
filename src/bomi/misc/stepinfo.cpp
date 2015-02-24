#include "stepinfo.hpp"

static QList<const StepInfo*> s_info;

StepInfo::StepInfo(const QByteArray &id, StepValue (Steps::*mem),
                     double max, double single, int precision,
                     const char *suffix, bool sign, int mul)
    : id(id), max(max), single(single), precision(precision)
    , multiply(mul), sign(sign), m_suffix(suffix), m_mem(mem)
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
    return m_info->text(prop);
}

Steps::Steps() {

#define STEP(name, val, ...) { \
    static const StepInfo info(#name, &Steps::name, __VA_ARGS__);\
    name = {val, &info}; }
#define TIME(name, tm, sng) STEP(name, tm, 999, sng, 3, QT_TR_NOOP("sec"), true, 1000)

    TIME(seek1_sec,  5, 1);
    TIME(seek2_sec, 30, 1);
    TIME(seek3_sec, 60, 1);
    STEP(speed, 10, 99, 1, 0, "%");

    STEP(aspect_ratio, 0.0001, 9.99999, 0.0001, 5, "");
    STEP(color, 1, 99, 1, 0, "%", true);

    STEP(volume, 1, 99, 1, 0, "%");
    STEP(amp, 10, 99, 1, 0, "%");
    TIME(audio_sync_sec, 0.2, 0.1);

    TIME(sub_sync_sec, 0.2, 0.1);
    STEP(sub_pos, 1, 99, 1, 0, "%");

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
    return true;
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
////    int dec = 0;
////    const auto exp = std::log10(qAbs(m_info.step()));
////    if (exp < 0)
////        dec = 1 + (int)-exp;
//    const auto info = this->info();
//    return _NS(_EnumData(direction) * m_step, true, info.prec) % info.suffix();
//}

auto StepInfo::text(double prop) const -> QString
{
    return _NS(prop/multiply, sign, precision) % suffix();
}

auto StepInfo::text(double step, ChangeValue direction, bool sign) const -> QString
{
    return _NS(_EnumData(direction) * step, sign, precision) % suffix();
}

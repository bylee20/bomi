#ifndef INTERPOLATORPARAMS_HPP
#define INTERPOLATORPARAMS_HPP

#include "enum/interpolator.hpp"

struct IntrplParam {
private:
    Q_DECLARE_TR_FUNCTIONS(IntrplParam)
public:
    enum Type {
        Radius, Param1, Param2, AntiRinging, TypeMax
    };
    IntrplParam() = default;
    auto operator == (const IntrplParam &rhs) const -> bool;
    auto operator != (const IntrplParam &rhs) const -> bool
        { return !operator == (rhs); }
    auto description() const -> QString;
    static auto description(Type type) -> QString;
    auto isAvailable() const -> bool { return m_available; }
    auto value() const -> double { return m_value; }
    auto toInt() const -> int { return qRound(m_value); }
    auto toDouble() const -> double { return m_value; }
    auto setInt(int value) -> void
    {
        if (m_validValues.contains(value))
            m_value = value;
        else
            qDebug() << "wrong value for IntrplParam";
    }
    auto setDouble(double value) -> void
        { m_value = qBound(m_min, value, m_max); }
    auto setValue(double value) -> void
        { if (isInt()) setInt(qRound(value)); else setDouble(value); }
    auto type() const -> Type { return m_type; }
    auto isInt() const -> bool { return !m_validValues.isEmpty(); }
    auto toByteArray() const -> QByteArray
        { return isInt() ? QByteArray::number(toInt()) : QByteArray::number(m_value);  }
    auto validValues() const -> const QVector<int>& { return m_validValues; }
    auto min() const -> double { return m_min; }
    auto max() const -> double { return m_max; }
private:
    IntrplParam(double value, double min = 0, double max = 1)
        : m_value(value), m_min(min), m_max(max), m_available(true) { }
    IntrplParam(int value, std::initializer_list<int> &&values)
        : m_value(value), m_validValues(std::move(values)), m_available(true) { }
    friend class IntrplParamSet;
    Type m_type;
    double m_value = -1, m_min = 0, m_max = 1;
    QVector<int> m_validValues;
    bool m_available = false;
};

struct IntrplParamSet {
    Interpolator type = Interpolator::Bilinear;
    std::array<IntrplParam, IntrplParam::TypeMax> params;
    auto operator == (const IntrplParamSet &rhs) const -> bool
        { return type == rhs.type && params == rhs.params; }
    auto operator != (const IntrplParamSet &rhs) const -> bool
        { return !operator == (rhs); }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto default_(Interpolator type) -> IntrplParamSet;
    static auto defaults() -> QMap<Interpolator, IntrplParamSet>;
    auto toMpvOption(const QByteArray &prefix) const -> QByteArray;
};

using IntrplParamSetMap = QMap<Interpolator, IntrplParamSet>;

class IntrplDialog : public QDialog {
    Q_OBJECT
public:
    IntrplDialog(QWidget *parent = nullptr);
    ~IntrplDialog();
    auto set(Interpolator intrpl, const IntrplParamSetMap &sets) -> void;
signals:
    void paramsChanged(const IntrplParamSet &params);
private:
    auto set(const IntrplParamSet &set) -> void;
    struct Data;
    Data *d;
};

#endif // INTERPOLATORPARAMS_HPP

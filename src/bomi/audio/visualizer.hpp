#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "quick/simpletextureitem.hpp"
#include "enum/visualization.hpp"

class AudioBuffer;

class AudioVisualizer : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QList<qreal> data READ data NOTIFY dataChanged)
    Q_PROPERTY(qreal min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_PROPERTY(Scale xScale READ xScale WRITE setXScale NOTIFY xScaleChanged)
    Q_PROPERTY(Scale yScale READ yScale WRITE setYScale NOTIFY yScaleChanged)
    Q_PROPERTY(Type type READ type NOTIFY typeChanged)
    Q_ENUMS(Scale)
    Q_ENUMS(Type)
public:
    enum Scale { Log, Linear };
    enum Type {
        None = (int)Visualization::Off,
        Bar = (int)Visualization::Bar
    };
    AudioVisualizer(QObject *parent = nullptr);
    ~AudioVisualizer();
    auto data() const -> QList<qreal>;
    auto count() const -> int;
    auto setCount(int count) -> void;
    auto min() const -> qreal;
    auto max() const -> qreal;
    auto setMin(qreal min) -> void;
    auto setMax(qreal max) -> void;
    auto isActive() const -> bool;
    auto setActive(bool active) -> void;
    auto isEnabled() const -> bool;
    auto xScale() const -> Scale;
    auto setXScale(Scale scale) -> void;
    auto yScale() const -> Scale;
    auto setYScale(Scale scale) -> void;
    auto setType(Visualization type) -> void;
    auto type() const -> Type;
    // in af thread
    auto analyze(const QSharedPointer<AudioBuffer> &data) -> void;
    auto reset() -> void;
signals:
    void audioChanged();
    void countChanged();
    void dataChanged();
    void minChanged();
    void maxChanged();
    void activeChanged();
    void enabledChanged();
    void xScaleChanged();
    void yScaleChanged();
    void typeChanged();
private:
    auto setEnabled(bool enabled) -> void;
    auto customEvent(QEvent *e) -> void final;
    struct Data;
    Data *d;
};

Q_DECLARE_METATYPE(AudioVisualizer::Scale)
Q_DECLARE_METATYPE(AudioVisualizer::Type)

#endif // VISUALIZER_HPP

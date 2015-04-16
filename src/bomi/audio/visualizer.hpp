#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "quick/simpletextureitem.hpp"

class AudioObject;

class VisualizationHelper : public QObject {
    Q_OBJECT
    Q_PROPERTY(AudioObject *audio READ audio WRITE setAudio NOTIFY audioChanged)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(QList<qreal> spectrum READ spectrum NOTIFY spectrumChanged)
    Q_PROPERTY(qreal min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
public:
    VisualizationHelper(QObject *parent = nullptr);
    ~VisualizationHelper();
    auto audio() const -> AudioObject*;
    auto setAudio(AudioObject *audio) -> void;
    auto spectrum() const -> QList<qreal>;
    auto setSpectrum(const QList<qreal> &spectrum) -> void;
    auto count() const -> int;
    auto setCount(int count) -> void;
    auto min() const -> qreal;
    auto max() const -> qreal;
    auto setMin(qreal min) -> void;
    auto setMax(qreal max) -> void;
    auto isActive() const -> bool;
    auto setActive(bool active) -> void;
signals:
    void audioChanged();
    void countChanged();
    void spectrumChanged();
    void minChanged();
    void maxChanged();
    void activeChanged();
private:
    struct Data;
    Data *d;
};

#endif // VISUALIZER_HPP

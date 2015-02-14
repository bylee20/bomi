#ifndef BUTTONBOXITEM_HPP
#define BUTTONBOXITEM_HPP

#include <QQuickItem>
#include <QDialogButtonBox>

class ButtonBoxItem : public QQuickItem {
    Q_OBJECT
    Q_ENUMS(Button)
    Q_PROPERTY(QQmlComponent *source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QList<int> buttons READ buttons WRITE setButtons NOTIFY buttonsChanged)
    Q_PROPERTY(qreal buttonWidth READ buttonWidth WRITE setButtonWidth NOTIFY buttonWidthChanged)
    Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY gapChanged)
    Q_PROPERTY(QQuickItem *clickedButton READ clickedButton NOTIFY clickedButtonChanged)
    using DBB = QDialogButtonBox;
public:
    enum Button { Ok = DBB::Ok, Cancel = DBB::Cancel, Yes = DBB::Yes, No = DBB::No };
    ButtonBoxItem(QQuickItem *parent = nullptr);
    ~ButtonBoxItem();
    QList<int> buttons() const;
    auto setButtons(QList<int> buttons) -> void;
    auto buttonWidth() const -> qreal;
    auto gap() const -> qreal;
    auto setButtonWidth(qreal w) -> void;
    auto setGap(qreal g) -> void;
    QQmlComponent *source() const;
    auto setSource(QQmlComponent *source) -> void;
    QQuickItem *clickedButton() const;
signals:
    void buttonsChanged();
    void gapChanged();
    void buttonWidthChanged();
    void sourceChanged();
    void clicked(int button);
    void clickedButtonChanged();
private slots:
    void emitClicked();
private:
    auto updatePolish() -> void;
    auto geometryChanged(const QRectF &new_, const QRectF &old) -> void;
    struct Data;
    Data *d;
};

#endif // BUTTONBOXITEM_HPP

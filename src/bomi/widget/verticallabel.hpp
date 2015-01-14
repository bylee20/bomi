#ifndef VERTICALLABEL_HPP
#define VERTICALLABEL_HPP

class VerticalLabel : public QFrame {
public:
    VerticalLabel(QWidget *parent = nullptr): QFrame(parent) { }
    VerticalLabel(const QString &text, QWidget *parent = nullptr)
        : QFrame(parent) { setText(text); }
    auto setText(const QString &text) -> void
        { if (_Change(m_text, text)) recalc(); }
    auto text() const -> QString { return m_text; }
    auto sizeHint() const -> QSize override { return minimumSizeHint(); }
    auto minimumSizeHint() const -> QSize override
        { return m_size.transposed(); }
protected:
    auto changeEvent(QEvent *event) -> void override;
    auto paintEvent(QPaintEvent *event) -> void override;
    auto recalc() -> void;
private:
    QString m_text;
    QSize m_size;
};


#endif // VERTICALLABEL_HPP

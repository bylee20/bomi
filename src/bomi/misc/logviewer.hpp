#ifndef LOGVIEWER_HPP
#define LOGVIEWER_HPP

class LogViewer : public QDialog {
    Q_OBJECT
public:
    LogViewer(QWidget *parent = nullptr);
    ~LogViewer();
private:
    auto customEvent(QEvent *ev) -> void final;
    auto showEvent(QShowEvent *event) -> void final;
    struct Data;
    Data *d;
};

#endif // LOGVIEW_HPP

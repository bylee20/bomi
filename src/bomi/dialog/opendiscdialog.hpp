#ifndef OPENDISCDIALOG_HPP
#define OPENDISCDIALOG_HPP

class OpenDiscDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(OpenDiscDialog)
public:
    OpenDiscDialog(QWidget *parent = 0);
    ~OpenDiscDialog();
    auto setDeviceList(const QStringList &devices) -> void;
    auto setDevice(const QString &device) -> void;
    auto setIsoEnabled(bool on) -> void;
    auto device() const -> QString;
    auto checkDevice(const QString &device) -> void;
private:
    struct Data;
    Data *d = nullptr;
};

#endif // OPENDISCDIALOG_HPP

#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

class AboutDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(AboutDialog)
public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
private:
    struct Data;
    Data *d = nullptr;
};

#endif // ABOUTDIALOG_HPP

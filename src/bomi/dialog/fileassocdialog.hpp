#ifndef FILEASSOCDIALOG_HPP
#define FILEASSOCDIALOG_HPP

class FileAssocDialog : public QDialog {
    Q_OBJECT
public:
    FileAssocDialog();
    ~FileAssocDialog();
private:
    struct Data;
    Data *d;
};

#endif // FILEASSOCDIALOG_HPP

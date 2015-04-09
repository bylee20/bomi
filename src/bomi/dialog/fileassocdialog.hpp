#ifndef FILEASSOCDIALOG_HPP
#define FILEASSOCDIALOG_HPP

class FileAssocDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(FileAssocDialog)
public:
    FileAssocDialog();
    ~FileAssocDialog();
private:
    struct Data;
    Data *d;
};

#endif // FILEASSOCDIALOG_HPP

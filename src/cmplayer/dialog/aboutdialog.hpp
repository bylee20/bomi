#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include "stdafx.hpp"

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
private:
    auto showFullLicense() -> void;
    struct Data;
    Data *d = nullptr;
};

#endif // ABOUTDIALOG_HPP

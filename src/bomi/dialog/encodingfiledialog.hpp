#ifndef ENCODINGFILEDIALOG_HPP
#define ENCODINGFILEDIALOG_HPP

#include <QFileDialog>
#include "misc/encodinginfo.hpp"

class EncodingComboBox;

class EncodingFileDialog : public QFileDialog {
    Q_DECLARE_TR_FUNCTIONS(EncodingFileDialog)
public:
    static auto getOpenFileName(QWidget *parent = 0,
                                const QString &caption = QString(),
                                const QString &dir = QString(),
                                const QString &filter = QString(),
                                EncodingInfo *enc = 0) -> QString;
    static auto getOpenFileNames(QWidget *parent = 0,
                                 const QString &caption = QString(),
                                 const QString &dir = QString(),
                                 const QString &filter = QString(),
                                 EncodingInfo *enc = 0,
                                 FileMode = ExistingFiles) -> QStringList;
private:
    EncodingFileDialog(QWidget *parent = 0,
                       const QString &caption = QString(),
                       const QString &directory = QString(),
                       const QString &filter = QString(),
                       const EncodingInfo &encoding = EncodingInfo());
    auto setEncoding(const EncodingInfo &encoding) -> void;
    auto encoding() const -> EncodingInfo;
    EncodingComboBox *combo = nullptr;
};

#endif // ENCODINGFILEDIALOG_HPP

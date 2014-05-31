#include "aboutdialog.hpp"
#include "ui_aboutdialog.h"

struct AboutDialog::Data {
    Ui::AboutDialog ui;
};

AboutDialog::AboutDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->ui.setupUi(this);
#define UI_LABEL_ARG(label, arg) d->ui.label->setText(d->ui.label->text().arg)
    UI_LABEL_ARG(version, arg(qApp->applicationVersion()));
    UI_LABEL_ARG(copyright, arg(QDate::currentDate().year()));
    UI_LABEL_ARG(contacts,
                 arg("<a href=\"http://xylosper.net\">"
                     "http://xylosper.net</a><br>"_a).
                 arg("<a href=\"mailto:darklin20@gmail.com\">"
                     "darklin20@gmail.com</a><br>"_a).
                 arg("<a href=\"http://cmplayer.github.com\">"
                     "http://cmplayer.github.com</a>"_a));
#undef UI_LABEL_ARG
    d->ui.license->setText(
        "This program is free software; "
        "you can redistribute it and/or modify it under the terms of "
        "the GNU General Public License "
        "as published by ""the Free Software Foundation; "
        "either version 2 of the License, "
        "or (at your option) any later version.<br><br>"

        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
        "See the GNU General Public License for more details.<br><br>"

        "You should have received a copy of "
        "the GNU General Public License along with this program; "
        "if not, see <a href=\"http://www.gnu.org/licenses\">"
        "http://www.gnu.org/licenses</a>.<br><br>"

        "Exception:<br>"
        "libchardet made by JoungKyun.Kim is "
        "distributed under Mozilla Public License(MPL)."_a
    );

    auto show = [this] ()
    {
        QDialog dlg(this);
        auto text = new QTextBrowser(&dlg);
        auto close = new QPushButton(tr("Close"), &dlg);
        auto vbox = new QVBoxLayout(&dlg);
        vbox->addWidget(text);
        auto hbox = new QHBoxLayout;
        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        hbox->addWidget(close);
        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        vbox->addLayout(hbox);
        connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);

        const QString fileName(sender() == d->ui.view_mpl ? ":/mpl.html"_a
                                                          : ":/gpl.html"_a);
        QFile file(fileName);
        file.open(QFile::ReadOnly | QFile::Text);
        text->setHtml(QString::fromLatin1(file.readAll()));
        dlg.resize(500, 400);
        dlg.exec();
    };
    connect(d->ui.view_gpl, &QPushButton::clicked, this, show);
    connect(d->ui.view_mpl, &QPushButton::clicked, this, show);

    setFixedHeight(420);
    setFixedWidth(width());
}

AboutDialog::~AboutDialog() {
    delete d;
}



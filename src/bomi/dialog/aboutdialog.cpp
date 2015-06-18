#include "aboutdialog.hpp"
#include "player/app.hpp"
#include "ui_aboutdialog.h"
#include <QTextBrowser>

struct AboutDialog::Data {
    Ui::AboutDialog ui;
};

AboutDialog::AboutDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->ui.setupUi(this);

    auto link = [] (const char *url) -> QString
        { return "<a href=\""_a % _L(url) % "\">"_a % _L(url) % "</a>"_a; };
    d->ui.app_name->setText(cApp.displayName());
#define UI_LABEL_ARG(label, arg) d->ui.label->setText(d->ui.label->text().arg)
    UI_LABEL_ARG(version, arg(_L(cApp.version())));
    UI_LABEL_ARG(qt_info, arg(_L(qVersion()), _L(QT_VERSION_STR)));
    UI_LABEL_ARG(copyright, arg(QDate::currentDate().year()).arg(tr("Lee, Byoung-young")));
    UI_LABEL_ARG(contacts, arg(link("http://bomi-player.github.io") % "<br>"_a).
                 arg(link("http://twitter.com/bomi_player") % "<br>"_a).
                 arg(link("https://github.com/xylosper/bomi/issues") % "<br>"_a).
                 arg("<a href=\"mailto:darklin20@gmail.com\">darklin20@gmail.com</a><br>"_a));
    UI_LABEL_ARG(ivan, arg(_L("https://plus.google.com/u/1/117118228830713086299/posts")));
#undef UI_LABEL_ARG
    d->ui.license->setText(
       u"This program is free software; "
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
        "http://www.gnu.org/licenses</a>."_q
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

        const QString fileName(u":/gpl.html"_q);
        QFile file(fileName);
        file.open(QFile::ReadOnly | QFile::Text);
        text->setHtml(QString::fromLatin1(file.readAll()));
        dlg.resize(500, 400);
        dlg.exec();
    };
    connect(d->ui.view_gpl, &QPushButton::clicked, this, show);

    adjustSize();
}

AboutDialog::~AboutDialog() {
    delete d;
}

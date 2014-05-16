#ifndef FONTOPTIONWIDGET_HPP
#define FONTOPTIONWIDGET_HPP

#include "stdafx.hpp"

class FontOptionWidget : public QWidget {
    Q_OBJECT
public:
    FontOptionWidget(QWidget *parent = 0);
    ~FontOptionWidget();
    auto set(const QFont &f) -> void
        {set(f.bold(), f.italic(), f.underline(), f.strikeOut());}
    auto set(bool bold, bool italic, bool underline, bool strikeout) -> void;
    auto bold() const -> bool;
    auto italic() const -> bool;
    auto underline() const -> bool;
    auto strikeOut() const -> bool;
    auto apply(QFont &font) -> void;
private:
    struct Data;
    Data *d;
};


#endif // FONTOPTIONWIDGET_HPP

#ifndef PATHBUTTON_HPP
#define PATHBUTTON_HPP

class PathButton : public QPushButton {
    Q_OBJECT
public:
    PathButton(QWidget *parent = nullptr);
    ~PathButton();
private:
    struct Data;
    Data *d;
};

#endif // PATHBUTTON_HPP

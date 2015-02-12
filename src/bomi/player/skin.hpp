#ifndef SKIN_HPP
#define SKIN_HPP

class QQuickView;

class Skin {
public:
    ~Skin() {}
    static auto dirs() -> QStringList {return data()->dirs;}
    static auto names(bool reload = false) -> QStringList;
    static auto source(const QString &name) -> QFileInfo;
    static auto apply(QQuickView *view, const QString &name) -> void;
protected:
    Skin() {}
private:
    struct Data {
        Data();
        QStringList dirs, qmls;
        QMap<QString, QFileInfo> skins;
    };
    static Data *data() { static Data data;    return &data; }
};

#endif // SKIN_HPP

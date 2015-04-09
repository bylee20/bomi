#ifndef JRIFACE_HPP
#define JRIFACE_HPP

class JrRequest;                        class JrResponse;

class JrIface : public QObject {
public:
    JrIface(QObject *parent = nullptr): QObject(parent) { }
    ~JrIface() = default;
    virtual auto request(const JrRequest &request) -> JrResponse = 0;
};

#endif // JRIFACE_HPP

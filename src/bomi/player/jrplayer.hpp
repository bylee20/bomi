#ifndef JRPLAYER_HPP
#define JRPLAYER_HPP

#include "json/jriface.hpp"

class JrPlayer : public JrIface {
public:
    JrPlayer(QObject *parent = nullptr);
    ~JrPlayer();
private:
    auto request(const JrRequest &request) -> JrResponse final;
    struct Data;
    Data *d;
};

#endif // JRIFACE_HPP

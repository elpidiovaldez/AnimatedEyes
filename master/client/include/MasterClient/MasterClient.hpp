#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <zmq.hpp>

class Master {
public:

    Master();
    ~Master();

    //Ensure master can't be moved or copied.
    Master(const Master &) = delete;
    Master &operator=(const Master &) = delete;
    Master(Master &&) = delete;
    Master &operator=(Master &&) = delete;

    std::string    lookup(std::string_view name);
    std::string    transformToBindAddress(std::string_view address);
    zmq::socket_t  connect(std::string_view name, zmq::socket_type type);
    zmq::socket_t  bind(std::string_view name, zmq::socket_type type);
    zmq::context_t &context();

    // How long lookup() waits for the Master before throwing.
    static constexpr int LOOKUP_TIMEOUT_MS = 2000;

private:

    friend Master &master();

    zmq::context_t zmqContext;
    zmq::socket_t  socket;
    std::mutex     lookupMutex;
    inline static Master *ptrToMaster = nullptr;
};

extern Master &master();

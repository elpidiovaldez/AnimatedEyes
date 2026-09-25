#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include "MasterClient/MasterClient.hpp"

Master::Master() : 
    zmqContext(1),
    socket(zmqContext, zmq::socket_type::req) {

    if (ptrToMaster != nullptr) {
        throw std::logic_error("Only one Master object may exist");
    }

    const char *address = std::getenv("MASTER");

    if (address == nullptr) {
        throw std::runtime_error("MASTER environment variable not set");
    }

    // Don't wait forever for a Master that isn't running. Relaxed/correlated REQ lets
    // the socket send a fresh request after a timed-out one, discarding any late reply.
    socket.set(zmq::sockopt::linger, 0);
    socket.set(zmq::sockopt::rcvtimeo, LOOKUP_TIMEOUT_MS);
    socket.set(zmq::sockopt::req_relaxed, 1);
    socket.set(zmq::sockopt::req_correlate, 1);

    socket.connect(address);

    ptrToMaster = this;
}
    
    Master::~Master() {
        ptrToMaster = nullptr;
    }

std::string Master::lookup(std::string_view name) {
    std::lock_guard<std::mutex> lock(lookupMutex);  //Deals with two threads simultaneously creating sockets.

    zmq::message_t request(name.data(), name.size());
    socket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    auto received = socket.recv(reply, zmq::recv_flags::none);
    if (!received) {
        throw std::runtime_error("ERROR: no reply from Master looking up " + std::string(name));
    }

    std::string response(static_cast<char *>(reply.data()), reply.size());
    if (response.rfind("ERROR:", 0) == 0) {
        throw std::runtime_error(response);
    }

    return response;
}

std::string Master::transformToBindAddress(std::string_view address) {

    if(address.starts_with("tcp://")) {
        size_t portStart = address.find(':', 6);

        if(portStart == std::string_view::npos) {
            throw std::runtime_error("TCP endpoint has no port");
        }

        return std::string("tcp://*") + std::string(address.substr(portStart));
    }

    if(address.starts_with("inproc://") || address.starts_with("ipc://")) {
        return std::string(address);
    }

    throw std::runtime_error("Unsupported ZeroMQ transport");
}

zmq::context_t &Master::context() {
    return zmqContext;
}

zmq::socket_t Master::connect(std::string_view name, zmq::socket_type type) {
    zmq::socket_t socket(zmqContext, type);
    socket.set(zmq::sockopt::linger, 0);
    socket.connect(lookup(name));
    return socket;
}

zmq::socket_t Master::bind(std::string_view name, zmq::socket_type type) {
    zmq::socket_t socket(zmqContext, type);
    socket.set(zmq::sockopt::linger, 0);
    socket.bind(transformToBindAddress(lookup(name)));
    return socket;
}

Master &master() {
    
    if (Master::ptrToMaster == nullptr) {
        throw std::logic_error("No Master object has been constructed");
    }

    return *Master::ptrToMaster;
}

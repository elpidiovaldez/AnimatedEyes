#pragma once

#include <memory>
#include <optional>
#include <string>
#include <zmq.hpp>
#include "EndpointResolver.h"
#include "ZmqUtil.h"

// Receives commands (e.g. "blink left") sent by other processes or machines over ZeroMQ.
// The commands are described in docs/command-protocol.md.

/// Listens for commands on a ZeroMQ PULL socket, without ever blocking the caller, so it can be
/// polled from the render loop. Any number of senders can connect a PUSH socket to it. Endpoint
/// lookup runs in the background (see EndpointResolver).
class CommandLink {
private:
  EndpointResolver resolver;
  std::unique_ptr<zmq::socket_t> socket;

  void tryBind() {
    auto address = resolver.poll();
    if (!address) {
      return;
    }
    try {
      std::string bindAddress = toBindAddress(*address);
      socket = std::make_unique<zmq::socket_t>(zmqContext(), zmq::socket_type::pull);
      socket->set(zmq::sockopt::linger, 0);
      socket->bind(bindAddress);
      resolver.report("listening on " + bindAddress);
    } catch (const std::exception &e) {
      socket.reset();
      resolver.failed(e.what());
    }
  }

public:
  /// \param endpoint a ZeroMQ address or Master endpoint name to bind to. Empty disables commands.
  explicit CommandLink(std::string endpoint) : resolver(std::move(endpoint), "Commands") {}

  /// \return false if no endpoint was configured for commands.
  bool isEnabled() const {
    return resolver.isEnabled();
  }

  /// Call regularly, e.g. once per frame. Never blocks.
  /// \return the next command waiting to be handled, if any.
  std::optional<std::string> poll() {
    if (!isEnabled()) {
      return std::nullopt;
    }
    if (!socket) {
      tryBind();
      if (!socket) {
        return std::nullopt;
      }
    }
    zmq::message_t message;
    if (socket->recv(message, zmq::recv_flags::dontwait)) {
      return message.to_string();
    }
    return std::nullopt;
  }
};

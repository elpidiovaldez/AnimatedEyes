#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <zmq.hpp>
#include "EndpointResolver.h"
#include "ZmqUtil.h"
#include "../Arduino.h"

// A non-blocking request/reply link for fetching sensor data from another process or
// machine over ZeroMQ. The wire protocol is described in docs/sensor-protocol.md.

/// Repeatedly requests data from a sensor provider without ever blocking the caller, so it can
/// be polled from the render loop. A request is sent every `intervalMs`; a request that gets no
/// reply within `timeoutMs` is abandoned, so a provider that stops or restarts is handled
/// automatically. Endpoint lookup runs in the background (see EndpointResolver).
class SensorLink {
private:
  static constexpr uint64_t LIVE_MS = 2'000;

  EndpointResolver resolver;
  std::string request;
  uint64_t intervalMs;
  uint64_t timeoutMs;
  std::unique_ptr<zmq::socket_t> socket;

  bool awaitingReply{false};
  uint64_t sentMs{};
  bool haveReply{false};
  uint64_t lastReplyMs{};

  void tryConnect() {
    auto address = resolver.poll();
    if (!address) {
      return;
    }
    try {
      socket = std::make_unique<zmq::socket_t>(zmqContext(), zmq::socket_type::req);
      socket->set(zmq::sockopt::linger, 0);
      // Only queue requests while actually connected, and allow a new request after an
      // unanswered one (discarding any late reply to the old one).
      socket->set(zmq::sockopt::immediate, 1);
      socket->set(zmq::sockopt::req_relaxed, 1);
      socket->set(zmq::sockopt::req_correlate, 1);
      socket->connect(*address);
      resolver.report("connecting to " + *address);
    } catch (const std::exception &e) {
      socket.reset();
      resolver.failed(e.what());
    }
  }

public:
  SensorLink(std::string endpoint, std::string request, uint64_t intervalMs, uint64_t timeoutMs = 500)
      : resolver(std::move(endpoint), "Sensor '" + request + "'"), request(std::move(request)), intervalMs(intervalMs),
        timeoutMs(timeoutMs) {}

  /// \return false if no endpoint was configured for this sensor.
  bool isEnabled() const {
    return resolver.isEnabled();
  }

  /// \return true if the provider has replied recently.
  bool isLive() const {
    return haveReply && millis() - lastReplyMs < LIVE_MS;
  }

  /// Call regularly, e.g. once per frame. Never blocks.
  /// \return a reply, if one has arrived since the last call. Error replies are logged, not returned.
  std::optional<std::string> poll() {
    if (!isEnabled()) {
      return std::nullopt;
    }
    if (!socket) {
      tryConnect();
      if (!socket) {
        return std::nullopt;
      }
    }

    uint64_t now = millis();
    if (awaitingReply) {
      zmq::message_t reply;
      if (socket->recv(reply, zmq::recv_flags::dontwait)) {
        awaitingReply = false;
        std::string text = reply.to_string();
        if (firstWord(text) == "error") {
          resolver.report(text);
          return std::nullopt;
        }
        if (!haveReply || now - lastReplyMs >= LIVE_MS) {
          resolver.report("receiving data");
        }
        haveReply = true;
        lastReplyMs = now;
        return text;
      }
      if (now - sentMs < timeoutMs) {
        return std::nullopt;
      }
      awaitingReply = false;
      if (!isLive()) {
        resolver.report("no reply");
      }
    }

    if (now - sentMs >= intervalMs) {
      sentMs = now;
      awaitingReply = socket->send(zmq::buffer(request), zmq::send_flags::dontwait).has_value();
      if (!awaitingReply && !isLive()) {
        resolver.report("waiting for provider");
      }
    }
    return std::nullopt;
  }
};

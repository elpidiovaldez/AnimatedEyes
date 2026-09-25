#pragma once

#include <chrono>
#include <cstdint>
#include <future>
#include <optional>
#include <string>
#include "ZmqUtil.h"
#include "../Arduino.h"

/// Looks up an endpoint (see resolveEndpoint()) in the background so the caller never blocks,
/// retrying after failures, and logs status messages without repeating them.
class EndpointResolver {
private:
  static constexpr uint64_t RETRY_MS = 10'000;

  std::string endpoint;
  std::string logPrefix;
  std::future<std::string> resolving;
  uint64_t retryAtMs{};
  std::string lastMessage;

public:
  /// \param endpoint a ZeroMQ address or Master endpoint name.
  /// \param logPrefix what to put before log messages, e.g. "Commands".
  EndpointResolver(std::string endpoint, const std::string &logPrefix)
      : endpoint(std::move(endpoint)), logPrefix(logPrefix + " (" + this->endpoint + "): ") {}

  /// \return false if no endpoint was configured.
  bool isEnabled() const {
    return !endpoint.empty();
  }

  /// Call regularly until it returns an address. Never blocks.
  /// \return the address, once a lookup has succeeded.
  std::optional<std::string> poll() {
    if (!resolving.valid()) {
      if (millis() >= retryAtMs) {
        resolving = std::async(std::launch::async, resolveEndpoint, endpoint);
      }
      return std::nullopt;
    }
    if (resolving.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      return std::nullopt;
    }
    try {
      return resolving.get();
    } catch (const std::exception &e) {
      failed(e.what());
      return std::nullopt;
    }
  }

  /// Logs `message` and makes poll() try again after a while.
  void failed(const std::string &message) {
    retryAtMs = millis() + RETRY_MS;
    report(message);
  }

  /// Logs `message`, unless it's the same as the last one.
  void report(const std::string &message) {
    if (message != lastMessage) {
      lastMessage = message;
      Serial.println(logPrefix + message);
    }
  }
};

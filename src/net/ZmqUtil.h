#pragma once

#include <stdexcept>
#include <string>
#include <zmq.hpp>
#include "MasterClient/MasterClient.hpp"
#include "Text.h"

// Helpers shared by the ZeroMQ sensor and command links. The message formats are described in
// docs/sensor-protocol.md and docs/command-protocol.md.

/// The ZeroMQ context shared by all links.
inline zmq::context_t &zmqContext() {
  static zmq::context_t context;
  return context;
}

/// Turns an endpoint setting into a ZeroMQ address. A setting containing "://" (e.g. "tcp://clio:5561")
/// is already an address; anything else is a name to look up with the Master service at $MASTER.
/// Throws if the lookup fails.
inline std::string resolveEndpoint(const std::string &endpoint) {
  if (endpoint.find("://") != std::string::npos) {
    return endpoint;
  }
  static Master master;
  return master.lookup(endpoint);
}

/// Turns an address into one suitable for binding: "tcp://host:port" becomes "tcp://*:port",
/// so the socket listens on all network interfaces. Other transports are unchanged.
inline std::string toBindAddress(const std::string &address) {
  if (address.starts_with("tcp://")) {
    size_t portStart = address.find(':', 6);
    if (portStart == std::string::npos) {
      throw std::runtime_error("TCP endpoint has no port: " + address);
    }
    return "tcp://*" + address.substr(portStart);
  }
  return address;
}

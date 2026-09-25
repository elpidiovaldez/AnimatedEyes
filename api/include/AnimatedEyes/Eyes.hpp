#pragma once

// A small function-based API for controlling AnimatedEyes from C++:
//
//     #include <AnimatedEyes/Eyes.hpp>
//     namespace eyes = animatedeyes;
//
//     eyes::blink();                               // blink both eyes
//     eyes::blink(eyes::Eye::Left);                // wink the left eye
//     eyes::open(0.0f);                            // close both eyes until changed
//     eyes::open(0.4f, eyes::Eye::Right);          // squint the right eye until changed
//     eyes::open(0.0f, 2.0f, eyes::Eye::Left);     // hold a left wink for 2 seconds, then open
//     eyes::open(1.0f);                            // open both eyes fully
//     eyes::blinkRate(20);                         // 20 automatic blinks per minute
//     eyes::blinkRate(0);                          // no automatic blinking
//     eyes::blinkRate();                           // default rate (10 per minute)
//     eyes::style("cat");                          // switch to the "cat" eye style
//     eyes::style("cycle");                        // change style every 10 seconds
//     eyes::send("blink right");                   // any command, as text
//     if (!eyes::blink()) { /* eyes not reachable */ }
//
//     // Optional settings, before sending any commands:
//     eyes::setEndpoint("tcp://asia:5562");        // connect directly instead of via the Master
//     eyes::setTimeout(std::chrono::milliseconds(500));   // wait up to 500 ms for the eyes
//
// The eyes are found by looking up "eyes.commands" with the Master name service at $MASTER, or
// at the address or name in $EYES_COMMAND_ENDPOINT if that is set (see setEndpoint()). If your
// program uses the Master itself, construct your Master object before calling these functions
// and they will share it. See docs/command-protocol.md for what each command does.
//
// Every function returns true if the command was sent, or false if the eyes couldn't be reached
// within the timeout (see setTimeout()). The functions are thread-safe.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <zmq.hpp>
#include "MasterClient/MasterClient.hpp"

namespace animatedeyes {

/// Which eye(s) a command applies to. Left is the robot's own left eye (display 0).
enum class Eye { Both, Left, Right };

namespace detail {

struct Connection {
  std::mutex mutex;
  std::string endpoint;          // Address or Master name to connect to
  std::optional<zmq::context_t> context;
  std::optional<zmq::socket_t> socket;
  int timeoutMs{200};
  std::chrono::steady_clock::time_point retryAt{};

  Connection() {
    const char *value = std::getenv("EYES_COMMAND_ENDPOINT");
    endpoint = value ? value : "eyes.commands";
  }
};

inline Connection &connection() {
  static Connection c;
  return c;
}

/// The program's Master client: the one it already has, or else one created here.
inline Master &sharedMaster() {
  try {
    return master();
  } catch (const std::logic_error &) {
    static Master ownMaster;
    return ownMaster;
  }
}

/// Connects if not already connected. Returns false (without retrying for a few seconds)
/// if the eyes' address can't be found, so a missing Master doesn't slow every call.
inline bool connect(Connection &c) {
  if (c.socket) {
    return true;
  }
  const auto now = std::chrono::steady_clock::now();
  if (now < c.retryAt) {
    return false;
  }
  try {
    std::string address = c.endpoint.find("://") != std::string::npos ? c.endpoint : sharedMaster().lookup(c.endpoint);
    if (auto star = address.find('*'); star != std::string::npos) {
      address.replace(star, 1, "localhost");   // A bind address like tcp://*:5562 means this machine
    }
    if (!c.context) {
      c.context.emplace();
    }
    zmq::socket_t socket(*c.context, zmq::socket_type::push);
    socket.set(zmq::sockopt::linger, c.timeoutMs);   // Let commands sent just before exit get through
    socket.set(zmq::sockopt::immediate, 1);   // Don't queue commands while the eyes aren't there
    socket.set(zmq::sockopt::sndtimeo, c.timeoutMs);
    socket.connect(address);
    c.socket.emplace(std::move(socket));
    return true;
  } catch (const std::exception &) {
    c.retryAt = now + std::chrono::seconds(5);
    return false;
  }
}

inline const char *eyeWord(Eye eye) {
  switch (eye) {
    case Eye::Left: return " left";
    case Eye::Right: return " right";
    default: return "";
  }
}

inline std::string number(float value) {
  char text[32];
  std::snprintf(text, sizeof(text), "%g", value);
  return text;
}

} // namespace detail

/// Sends a command in the text form described in docs/command-protocol.md, e.g. "open 0.3 left".
inline bool send(std::string_view command) {
  detail::Connection &c = detail::connection();
  std::lock_guard<std::mutex> lock(c.mutex);
  if (!detail::connect(c)) {
    return false;
  }
  try {
    return c.socket->send(zmq::buffer(command), zmq::send_flags::none).has_value();
  } catch (const zmq::error_t &) {
    return false;
  }
}

/// Blinks both eyes, or winks one.
inline bool blink(Eye eye = Eye::Both) {
  return send(std::string("blink") + detail::eyeWord(eye));
}

/// Moves the eyelids to `level`, from 0 (closed) to 1 (fully open), until changed.
inline bool open(float level, Eye eye = Eye::Both) {
  return send("open " + detail::number(level) + detail::eyeWord(eye));
}

/// Moves the eyelids to `level`, from 0 (closed) to 1 (fully open), for `seconds`, then opens them fully.
inline bool open(float level, float seconds, Eye eye = Eye::Both) {
  return send("open " + detail::number(level) + " " + detail::number(seconds) + detail::eyeWord(eye));
}

/// Sets the average number of automatic blinks per minute. 0 turns automatic blinking off.
inline bool blinkRate(float perMinute = 10.0f) {
  return send("blink_rate " + detail::number(perMinute));
}

/// Switches to the named eye style (e.g. "cat"), or "cycle" to change style periodically.
inline bool style(std::string_view name) {
  return send("style " + std::string(name));
}

/// Sets where to find the eyes: a ZeroMQ address such as "tcp://asia:5562", or a Master
/// endpoint name. Call before sending any commands.
inline void setEndpoint(std::string endpoint) {
  detail::Connection &c = detail::connection();
  std::lock_guard<std::mutex> lock(c.mutex);
  c.endpoint = std::move(endpoint);
  c.socket.reset();
  c.retryAt = {};
}

/// Sets how long a command may wait for the eyes to be reachable before giving up (default 200 ms).
/// Call before sending any commands.
inline void setTimeout(std::chrono::milliseconds timeout) {
  detail::Connection &c = detail::connection();
  std::lock_guard<std::mutex> lock(c.mutex);
  c.timeoutMs = static_cast<int>(timeout.count());
  c.socket.reset();
}

} // namespace animatedeyes

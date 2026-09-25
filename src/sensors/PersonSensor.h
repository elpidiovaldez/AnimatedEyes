#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "../net/SensorLink.h"

/// A face reported by a person sensor. Positions and sizes are fractions of the camera image:
/// (0, 0) is its top-left corner and (1, 1) its bottom-right.
struct Face {
  float x{};              ///< Centre of the face.
  float y{};
  float w{};              ///< Size of the face, or 0 if the sensor doesn't report it.
  float h{};
  float confidence{1.0f}; ///< How sure the sensor is that this is a face, from 0 to 1.
  bool facing{true};      ///< Whether the person is facing the camera.
  std::string id;         ///< Name or ID of a recognised person, if any.
};

/// Fetches face detections from a person sensor provider over ZeroMQ (see docs/sensor-protocol.md).
class PersonSensor {
private:
  static constexpr uint64_t SAMPLE_TIME_MS = 70;

  /// Results older than this (as reported by the provider) are treated as "no faces".
  static constexpr float MAX_AGE_MS = 1000;

  SensorLink link;
  std::vector<Face> detected;

public:
  /// \param endpoint a ZeroMQ address or Master endpoint name. Empty disables the sensor.
  explicit PersonSensor(std::string endpoint) : link(std::move(endpoint), "faces", SAMPLE_TIME_MS) {}

  bool isEnabled() const {
    return link.isEnabled();
  }

  /// \return true if the provider is currently answering.
  bool isLive() const {
    return link.isLive();
  }

  /// Checks for new results from the provider. Never blocks.
  /// \return true if a new set of results arrived, which faces() now holds.
  bool read() {
    auto reply = link.poll();
    if (!reply || firstWord(*reply) != "faces") {
      return false;
    }

    detected.clear();
    std::string_view text = *reply;
    size_t lineEnd = text.find('\n');
    if (fieldFloat(parseFields(text.substr(0, lineEnd)), "age_ms", 0) > MAX_AGE_MS) {
      return true;
    }

    while (lineEnd != std::string_view::npos) {
      text.remove_prefix(lineEnd + 1);
      lineEnd = text.find('\n');
      Fields fields = parseFields(text.substr(0, lineEnd));
      if (!fields.contains("x") || !fields.contains("y")) {
        continue;
      }
      Face face;
      face.x = fieldFloat(fields, "x", 0.5f);
      face.y = fieldFloat(fields, "y", 0.5f);
      face.w = fieldFloat(fields, "w", 0);
      face.h = fieldFloat(fields, "h", 0);
      face.confidence = fieldFloat(fields, "conf", 1.0f);
      face.facing = fieldFloat(fields, "facing", 1.0f) != 0;
      if (auto id = fields.find("id"); id != fields.end()) {
        face.id = id->second;
      }
      detected.push_back(face);
    }
    return true;
  }

  /// \return the faces found in the most recent results.
  const std::vector<Face> &faces() const {
    return detected;
  }
};

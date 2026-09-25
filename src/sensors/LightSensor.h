#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include "../net/SensorLink.h"

/// Fetches the ambient light level from a light sensor provider over ZeroMQ (see docs/sensor-protocol.md).
class LightSensor {
private:
  static constexpr uint64_t SAMPLE_TIME_MS = 100;

  /// Readings older than this (as reported by the provider) are ignored.
  static constexpr float MAX_AGE_MS = 2000;

  SensorLink link;
  float minLevel{};
  float maxLevel{};
  float curve{};
  float damping{};
  float previousValue{-1.0f};

public:
  /// \param endpoint a ZeroMQ address or Master endpoint name. Empty disables the sensor.
  /// \param minLevel reported level treated as fully dark; lower levels are clamped to it.
  /// \param maxLevel reported level treated as fully bright; higher levels are clamped to it.
  /// \param curve exponent applied after scaling to 0-1; above 1 makes the response less sensitive in dim light.
  /// \param damping how much of the previous value is kept at each new reading, from 0 (none) to just under 1.
  explicit LightSensor(std::string endpoint, float minLevel = 0.0f, float maxLevel = 1.0f, float curve = 1.0f,
                       float damping = 0.7f)
      : link(std::move(endpoint), "light", SAMPLE_TIME_MS), minLevel(minLevel), maxLevel(maxLevel), curve(curve),
        damping(damping) {}

  bool isEnabled() const {
    return link.isEnabled();
  }

  /// \return true if the provider is currently answering.
  bool isLive() const {
    return link.isLive();
  }

  /// Checks for a new reading and, if there is one, passes the damped light level
  /// (0 = dark, 1 = bright) to `processor`. Never blocks.
  void readDamped(std::function<void(float)> const &processor) {
    auto reply = link.poll();
    if (!reply || firstWord(*reply) != "light") {
      return;
    }
    Fields fields = parseFields(*reply);
    float level = fieldFloat(fields, "level", -1.0f);
    if (level < 0 || fieldFloat(fields, "age_ms", 0) > MAX_AGE_MS) {
      return;
    }

    level = std::pow(std::clamp((level - minLevel) / (maxLevel - minLevel), 0.0f, 1.0f), curve);
    previousValue = previousValue < 0 ? level : damping * previousValue + (1.0f - damping) * level;
    processor(previousValue);
  }
};

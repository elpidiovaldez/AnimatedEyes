#pragma once

#include <cstdint>
#include <functional>

class LightSensor {
private:
  uint32_t minReading{};
  uint32_t maxReading{};
  float curve{};
  uint32_t lastReadTimeMs{};
  float previousValue{0.5f};

public:
  explicit LightSensor(uint32_t minReading = 0, uint32_t maxReading = 1023, float curve = 1.0f)
      : minReading(minReading), maxReading(maxReading), curve(curve) {}

  bool isEnabled() const {
    return false;
  }

  void readDamped(std::function<void(float)> const& processor) {}
};

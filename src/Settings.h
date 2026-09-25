#pragma once

#include <cstdlib>
#include <string>

// Settings from config.h that can be overridden at run time with environment variables.

/// \return the value of environment variable `envVar` if it is set (even to ""), otherwise `fallback`.
inline std::string stringSetting(const char *envVar, const char *fallback) {
  const char *value = std::getenv(envVar);
  return value ? value : fallback;
}

/// \return the value of environment variable `envVar` as a number if it is set and valid, otherwise `fallback`.
inline float floatSetting(const char *envVar, float fallback) {
  const char *value = std::getenv(envVar);
  char *end{};
  float result = value ? std::strtof(value, &end) : fallback;
  return value && end != value ? result : fallback;
}

/// \return false if environment variable `envVar` is "0", true if it is "1", otherwise `fallback`.
inline bool boolSetting(const char *envVar, bool fallback) {
  const std::string value = stringSetting(envVar, "");
  return value == "0" ? false : value == "1" ? true : fallback;
}

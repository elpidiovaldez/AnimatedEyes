#pragma once

#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// Helpers for the text messages used by the sensor and command protocols
// (docs/sensor-protocol.md and docs/command-protocol.md).

/// The key=value fields from one line of a message, e.g. "x=0.5 y=0.25 id=paul".
/// Words without an '=' are skipped.
using Fields = std::map<std::string, std::string, std::less<>>;

inline Fields parseFields(std::string_view line) {
  Fields fields;
  size_t pos = 0;
  while (pos < line.size()) {
    size_t end = line.find_first_of(" \t\r", pos);
    if (end == std::string_view::npos) {
      end = line.size();
    }
    std::string_view word = line.substr(pos, end - pos);
    size_t eq = word.find('=');
    if (eq != std::string_view::npos && eq > 0) {
      fields.emplace(word.substr(0, eq), word.substr(eq + 1));
    }
    pos = end + 1;
  }
  return fields;
}

/// \return the named field as a float, or `fallback` if it is missing or not a number.
inline float fieldFloat(const Fields &fields, std::string_view key, float fallback) {
  auto it = fields.find(key);
  if (it == fields.end()) {
    return fallback;
  }
  char *end{};
  float value = std::strtof(it->second.c_str(), &end);
  return end == it->second.c_str() ? fallback : value;
}

/// \return the words of `line`, split at spaces, tabs and line breaks.
inline std::vector<std::string_view> splitWords(std::string_view line) {
  std::vector<std::string_view> words;
  size_t pos = line.find_first_not_of(" \t\r\n");
  while (pos != std::string_view::npos) {
    size_t end = line.find_first_of(" \t\r\n", pos);
    words.push_back(line.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos));
    pos = line.find_first_not_of(" \t\r\n", end);
  }
  return words;
}

/// \return the first word of `line`, e.g. "faces" for "faces age_ms=40".
inline std::string_view firstWord(std::string_view line) {
  return line.substr(0, line.find_first_of(" \t\r\n"));
}

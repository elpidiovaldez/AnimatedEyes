#pragma once

#include <charconv>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "net/Text.h"

/// A command for the eyes, e.g. "blink left" or "open 0.3 5". See docs/command-protocol.md.
struct Command {
  enum class Type { Blink, Open, BlinkRate, Style };

  Type type{};
  bool left{true};             ///< Whether the command applies to the left eye
  bool right{true};            ///< Whether the command applies to the right eye
  float level{};               ///< Open: how far open, 0 to 1
  float holdSeconds{};         ///< Open: how long to hold before opening fully, 0 = until changed
  std::optional<float> rate;   ///< BlinkRate: blinks per minute, if given
  std::string style;           ///< Style: the style name, or "cycle"
};

/// Parses a command. \return the command, or a description of what is wrong with it.
inline std::expected<Command, std::string> parseCommand(std::string_view text) {
  const std::vector<std::string_view> words = splitWords(text);
  if (words.empty()) {
    return std::unexpected("Empty command");
  }
  const std::string_view name = words[0];
  Command command;

  if (name == "style") {
    if (words.size() != 2) {
      return std::unexpected("'style' needs one name");
    }
    command.type = Command::Type::Style;
    command.style = words[1];
    return command;
  }

  // "left", "right" or "both" chooses the eyes; other arguments are numbers
  bool eyeGiven = false;
  std::vector<float> numbers;
  for (size_t i = 1; i < words.size(); i++) {
    const std::string_view word = words[i];
    float value{};
    if (word == "left" || word == "right" || word == "both") {
      command.left = word != "right";
      command.right = word != "left";
      eyeGiven = true;
    } else if (auto [end, error] = std::from_chars(word.data(), word.data() + word.size(), value);
               error == std::errc{} && end == word.data() + word.size()) {
      numbers.push_back(value);
    } else {
      return std::unexpected("Bad argument '" + std::string(word) + "'");
    }
  }

  if (name == "blink") {
    if (!numbers.empty()) {
      return std::unexpected("'blink' takes no numbers");
    }
    command.type = Command::Type::Blink;
  } else if (name == "open") {
    if (numbers.empty() || numbers.size() > 2 || numbers[0] < 0 || (numbers.size() == 2 && numbers[1] < 0)) {
      return std::unexpected("'open' needs a level of 0 or more, and optionally seconds of 0 or more");
    }
    command.type = Command::Type::Open;
    command.level = numbers[0];
    command.holdSeconds = numbers.size() == 2 ? numbers[1] : 0.0f;
  } else if (name == "blink_rate") {
    if (eyeGiven || numbers.size() > 1 || (numbers.size() == 1 && numbers[0] < 0)) {
      return std::unexpected("'blink_rate' takes an optional rate of 0 or more");
    }
    command.type = Command::Type::BlinkRate;
    if (!numbers.empty()) {
      command.rate = numbers[0];
    }
  } else {
    return std::unexpected("Unknown command");
  }
  return command;
}

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include "config.h"
#include "Commands.h"
#include "Settings.h"
#include "net/CommandLink.h"
#include "sensors/LightSensor.h"
#include "sensors/PersonSensor.h"

// Settings from config.h, which environment variables can override (see config.h)
const uint64_t styleDurationMs = static_cast<uint64_t>(floatSetting("EYES_STYLE_DURATION_MS", STYLE_DURATION_MS));
const bool mirrorPersonX = boolSetting("EYES_MIRROR_PERSON_X", MIRROR_PERSON_X);

LightSensor lightSensor(stringSetting("EYES_LIGHT_ENDPOINT", LIGHT_ENDPOINT),
                        floatSetting("EYES_LIGHT_MIN", LIGHT_MIN),
                        floatSetting("EYES_LIGHT_MAX", LIGHT_MAX),
                        floatSetting("EYES_LIGHT_CURVE", LIGHT_CURVE));
PersonSensor personSensor(stringSetting("EYES_PERSON_ENDPOINT", PERSON_ENDPOINT));
CommandLink commands(stringSetting("EYES_COMMAND_ENDPOINT", COMMAND_ENDPOINT));

// The index of the currently selected eye definitions
static size_t defIndex{0};

// Whether to change eye style every styleDurationMs, and when the style last changed
static bool cycleStyles{true};
static uint64_t styleTimeMs{0};

// When the eyes last had a face to follow
static uint64_t lastTargetMs{0};

/// \return the names of all the eye styles, separated by spaces.
std::string styleNames() {
  std::string names;
  for (const auto &definitions : eyeDefinitions) {
    names += std::string(names.empty() ? "" : " ") + definitions[0].name;
  }
  return names;
}

/// Switches to the named eye style, or with "cycle" goes back to changing style periodically.
void setStyle(std::string_view name) {
  if (name == "cycle") {
    cycleStyles = true;
    styleTimeMs = millis();
    return;
  }
  for (size_t i = 0; i < eyeDefinitions.size(); i++) {
    if (name == eyeDefinitions[i][0].name) {
      cycleStyles = false;
      defIndex = i;
      eyes->updateDefinitions(eyeDefinitions[i]);
      return;
    }
  }
  Serial.println("Unknown eye style '" + std::string(name) + "'. Styles are: " + styleNames() + " (or cycle)");
}

void nextEye() {
  defIndex = (defIndex + 1) % eyeDefinitions.size();
  eyes->updateDefinitions(eyeDefinitions.at(defIndex));
}

/// Carries out a command received over ZeroMQ, e.g. "blink left". See docs/command-protocol.md.
void handleCommand(const std::string &text) {
  const auto command = parseCommand(text);
  if (!command) {
    Serial.println(command.error() + ": " + text);
    return;
  }

  // eyes[0] is the left eye, eyes[1] the right
  switch (command->type) {
    case Command::Type::Blink:
      if (command->left && command->right) {
        eyes->blink();
      } else {
        eyes->wink(command->left ? 0 : 1);
      }
      break;
    case Command::Type::Open: {
      const auto holdMs = static_cast<uint32_t>(command->holdSeconds * 1000.0f);
      if (command->left) {
        eyes->setOpenness(0, command->level, holdMs);
      }
      if (command->right) {
        eyes->setOpenness(1, command->level, holdMs);
      }
      break;
    }
    case Command::Type::BlinkRate:
      eyes->setBlinkRate(command->rate.value_or(DEFAULT_BLINK_RATE));
      break;
    case Command::Type::Style:
      setStyle(command->style);
      break;
  }
}

/// Makes the eyes follow the closest (largest) face facing the camera, if there is one.
void followFaces() {
  const Face *target = nullptr;
  float maxSize = -1.0f;
  for (const Face &face : personSensor.faces()) {
    if (face.facing && face.confidence >= MIN_FACE_CONFIDENCE && face.w * face.h > maxSize) {
      maxSize = face.w * face.h;
      target = &face;
    }
  }
  if (target) {
    // Map image coordinates (0 to 1) to eye coordinates (-1 to 1), aiming a
    // little above the centre of the face, at the person's eyes.
    lastTargetMs = millis();
    eyes->setAutoMove(false);
    const float targetX = target->x * 2.0f - 1.0f;
    const float targetY = (target->y - target->h / 6.0f) * 2.0f - 1.0f;
    eyes->setTargetPosition(mirrorPersonX ? -targetX : targetX, targetY);
  }
}

int main() {
  Serial.println("Init");
  // Pupils resize automatically until light level data arrives
  initEyes(true, true, true);
  eyes->setBlinkRate(DEFAULT_BLINK_RATE);
  Serial.println("Eye styles: " + styleNames());

  while (true) {
    // Switch eyes periodically
    if (cycleStyles && millis() - styleTimeMs > styleDurationMs) {
      styleTimeMs = millis();
      nextEye();
    }

    // Handle any commands that have arrived (a few per frame at most, to keep the animation smooth)
    for (int i = 0; i < 10; i++) {
      auto command = commands.poll();
      if (!command) {
        break;
      }
      handleCommand(*command);
    }

    if (lightSensor.isEnabled()) {
      // Bright light makes the pupils small. Fall back to automatic resizing if the data stops.
      lightSensor.readDamped([](float level) {
        eyes->setPupil(1.0f - level);
      });
      eyes->setAutoPupils(!lightSensor.isLive());
    }

    if (personSensor.isEnabled()) {
      if (personSensor.read()) {
        followFaces();
      }
      if (millis() - lastTargetMs > 5'000 && !eyes->autoMoveEnabled()) {
        // We haven't had a face to follow for a while (or the sensor stopped) so enable automove
        eyes->setAutoMove(true);
      }
    }

    eyes->renderFrame();
  }
}

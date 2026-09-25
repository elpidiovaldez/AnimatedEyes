// Define if you wish to debug memory usage.  Only works on T4.x
//#define DEBUG_MEMORY

#include <array>
#include "config.h"
#include "sensors/LightSensor.h"
#include "sensors/PersonSensor.h"

// The index of the currently selected eye definitions
static uint32_t defIndex{0};

LightSensor lightSensor;
PersonSensor personSensor;
bool personSensorFound = USE_PERSON_SENSOR;

bool hasBlinkButton() {
  return false;
}

bool hasLightSensor() {
  return false;
}

bool hasJoystick() {
  return false;
}

bool hasPersonSensor() {
  return false;
}

/// INITIALIZATION -- runs once at startup ----------------------------------
void setup() {
  Serial.println("Init");

  if (hasPersonSensor()) {
      personSensorFound = personSensor.isPresent();
      if (personSensorFound) {
          Serial.println("Person Sensor detected");
          personSensor.enableID(false);
          personSensor.setMode(PersonSensor::Mode::Continuous);
        } else {
          Serial.println("No Person Sensor was found!");
        }
    }

  initEyes(!hasJoystick(), !hasBlinkButton(), !hasLightSensor());
}

void nextEye() {
  defIndex = (defIndex + 1) % eyeDefinitions.size();
  eyes->updateDefinitions(eyeDefinitions.at(defIndex));
}

/// MAIN LOOP -- runs continuously after setup() ----------------------------
int main() {
    
    setup();
    
    do {
        // Switch eyes periodically
        static long int eyeTime = 0;
        if (millis() - eyeTime > EYE_DURATION_MS) {
            eyeTime = millis();
            nextEye();
        }

        // Blink on button press
        if (hasBlinkButton() /* && blinkButtonPressed() */) {
            eyes->blink();
        }

        // Move eyes with an analog joystick
        if (hasJoystick()) {
            //Read joystick
            auto x = 0;
            auto y = 0;
            eyes->setPosition((x - 512) / 512.0f, (y - 512) / 512.0f);
        }

        if (hasLightSensor()) {
            lightSensor.readDamped([](float value) {
                eyes->setPupil(value);
            });
        }

        if (hasPersonSensor() && personSensor.read()) {
            // Find the closest face that is facing the camera, if any
            int maxSize = 0;
            person_sensor_face_t maxFace{};

            for (int i = 0; i < personSensor.numFacesFound(); i++) {
                const person_sensor_face_t face = personSensor.faceDetails(i);
                if (face.is_facing && face.box_confidence > 60) {
                    int size = (face.box_right - face.box_left) * (face.box_bottom - face.box_top);
                    if (size > maxSize) {
                        maxSize = size;
                        maxFace = face;
                    }
                }
            }

            if (maxSize > 0) {
                eyes->setAutoMove(false);
                float targetX = -((static_cast<float>(maxFace.box_left) + static_cast<float>(maxFace.box_right - maxFace.box_left) / 2.0f) / 127.5f - 1.0f);
                float targetY = (static_cast<float>(maxFace.box_top) + static_cast<float>(maxFace.box_bottom - maxFace.box_top) / 3.0f) / 127.5f - 1.0f;
                eyes->setTargetPosition(targetX, targetY);
            } else if (personSensor.timeSinceFaceDetectedMs() > 5'000 && !eyes->autoMoveEnabled()) {
                // We haven't seen a face for a while so enable automove
                eyes->setAutoMove(true);
            }
        }

        eyes->renderFrame();
    } while(true);

    return 0;
}

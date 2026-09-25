// Demonstrates the AnimatedEyes C++ API. Needs $MASTER set, or $EYES_COMMAND_ENDPOINT
// set to the eyes' address (e.g. tcp://asia:5562).

#include <chrono>
#include <iostream>
#include <thread>
#include <AnimatedEyes/Eyes.hpp>

using namespace std::chrono_literals;
namespace eyes = animatedeyes;

int main() {
  if (!eyes::style("cat")) {
    std::cerr << "Couldn't reach the eyes\n";
    return 1;
  }
  std::this_thread::sleep_for(1s);

  eyes::blink();
  std::this_thread::sleep_for(1s);

  eyes::open(0.0f, 1.0f, eyes::Eye::Left);   // Hold a left wink for a second
  std::this_thread::sleep_for(2s);

  eyes::open(0.4f);                          // Squint...
  std::this_thread::sleep_for(2s);
  eyes::open(1.0f);                          // ...and open again

  eyes::blinkRate();                         // Back to the default blink rate
  eyes::style("cycle");
  return 0;
}

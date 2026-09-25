// Tests for the command and message parsing. Build and run with: ctest --test-dir build

#include <iostream>
#include <string>
#include "Commands.h"
#include "net/Text.h"

static int failures = 0;

#define CHECK(condition)                                                        \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::cerr << __FILE__ << ":" << __LINE__ << ": FAILED: " #condition "\n"; \
      failures++;                                                               \
    }                                                                           \
  } while (false)

static void testText() {
  CHECK(firstWord("faces age_ms=40\nx=1") == "faces");
  CHECK(firstWord("light") == "light");

  auto words = splitWords("  open\t0.3   5 left ");
  CHECK(words.size() == 4 && words[0] == "open" && words[3] == "left");
  CHECK(splitWords("   ").empty());

  Fields fields = parseFields("x=0.5 y=0.25 id=paul junk =bad");
  CHECK(fields.size() == 3);
  CHECK(fieldFloat(fields, "x", -1) == 0.5f);
  CHECK(fieldFloat(fields, "id", -1) == -1);       // not a number
  CHECK(fieldFloat(fields, "missing", 7) == 7);
}

static void testCommands() {
  auto blink = parseCommand("blink");
  CHECK(blink && blink->type == Command::Type::Blink && blink->left && blink->right);

  auto wink = parseCommand("blink right");
  CHECK(wink && !wink->left && wink->right);

  auto open = parseCommand("open 0.3 5 left");
  CHECK(open && open->type == Command::Type::Open && open->level == 0.3f && open->holdSeconds == 5.0f);
  CHECK(open && open->left && !open->right);

  auto hold = parseCommand("open 0");
  CHECK(hold && hold->level == 0 && hold->holdSeconds == 0);

  auto rate = parseCommand("blink_rate 20");
  CHECK(rate && rate->type == Command::Type::BlinkRate && rate->rate == 20.0f);
  auto defaultRate = parseCommand("blink_rate");
  CHECK(defaultRate && !defaultRate->rate);

  auto style = parseCommand("style cat");
  CHECK(style && style->type == Command::Type::Style && style->style == "cat");

  // Malformed commands
  CHECK(!parseCommand(""));
  CHECK(!parseCommand("wibble"));
  CHECK(!parseCommand("blink 3"));
  CHECK(!parseCommand("blink sideways"));
  CHECK(!parseCommand("open"));
  CHECK(!parseCommand("open -1"));
  CHECK(!parseCommand("open 0.5 -3"));
  CHECK(!parseCommand("open 1 2 3"));
  CHECK(!parseCommand("blink_rate -5"));
  CHECK(!parseCommand("blink_rate 10 left"));
  CHECK(!parseCommand("style"));
  CHECK(!parseCommand("style cat dog"));
}

int main() {
  testText();
  testCommands();
  std::cout << (failures ? "FAILED" : "All tests passed") << "\n";
  return failures ? 1 : 0;
}

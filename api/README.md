# AnimatedEyes C++ API

An optional, header-only C++ API for controlling AnimatedEyes from other programs,
without dealing with ZeroMQ directly. Each function sends one of the commands in
[docs/command-protocol.md](../docs/command-protocol.md).

```cpp
#include <AnimatedEyes/Eyes.hpp>

animatedeyes::blink();                                      // blink both eyes
animatedeyes::blink(animatedeyes::Eye::Right);              // wink
animatedeyes::open(0.0f);                                   // close both eyes until changed
animatedeyes::open(0.0f, 2.0f, animatedeyes::Eye::Left);    // hold a left wink for 2 s
animatedeyes::blinkRate(0);                                 // stop automatic blinking
animatedeyes::style("leopard");                             // or "cycle"
animatedeyes::send("blink left");                           // any command as text
```

Every function returns `true` if the command was sent, or `false` if the eyes
couldn't be reached within the timeout (200 ms by default, see `setTimeout()`).
The functions are thread-safe.

## Finding the eyes

By default the eyes' address is found by looking up `eyes.commands` with the
[Master](../master/README.md) at `$MASTER`. Set `$EYES_COMMAND_ENDPOINT`, or call
`animatedeyes::setEndpoint()`, to use a direct address such as `tcp://asia:5562`
or a different Master name.

If your program uses the Master itself (like BDX programs do), construct your
`Master` object before calling the API and it will be shared.

## Building

Needs `libzmq3-dev` and `cppzmq-dev`. In your CMake project:

```cmake
add_subdirectory(/path/to/AnimatedEyes/api animatedeyes_api)
target_link_libraries(your_program PRIVATE AnimatedEyes::api)
```

If your project already has a Master client library target (`masterclient` or
`MasterClient::masterclient`), define it before `add_subdirectory` and the API
will use it; otherwise it builds the copy in `master/client`.

To build the example on its own:

```shell
cmake -S api -B build/api && cmake --build build/api
MASTER=tcp://clio:6000 build/api/eyes_example
```

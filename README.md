# AnimatedEyes

AnimatedEyes renders a pair of animated eyes on two round 240x240 GC9A01 LCD screens driven by a
Raspberry Pi running Linux. The eyes blink, glance around and change pupil size by themselves, and
can also follow people's faces, react to the ambient light level, and be controlled by other
programs (blink, wink, close, change style) over the network using [ZeroMQ](https://zeromq.org/).
23 eye styles are included, from realistic to cat, dragon and hypnotic.

### Credits

AnimatedEyes is based on [TeensyEyes](https://github.com/chrismiller/TeensyEyes) by Chris Miller, used under
the MIT License, ported from Teensy microcontrollers to Linux. Many thanks to Chris for the original work.
His introduction follows.

> This project is my take on "[Uncanny Eyes](https://github.com/adafruit/Uncanny_Eyes)", adapted for [Teensy 4.x](https://www.pjrc.com/store/teensy40.html)
> microcontrollers. It renders animated eyes on round GC9A01 LCD screens.
>
> This codebase is based on code and ideas from the following projects:
>   - https://github.com/adafruit/Uncanny_Eyes
>   - https://github.com/adafruit/Adafruit_Learning_System_Guides/tree/main/M4_Eyes
>   - https://github.com/mjs513/GC9A01A_t3n

The Linux port draws on the screens using [Display_Lib_RPI](https://github.com/gavinlyonsrepo/Display_Lib_RPI)
by Gavin Lyons, used under the MIT License. Many thanks to Gavin for his display library.

### Building

You need a Raspberry Pi with SPI enabled (`sudo raspi-config`, Interface Options, SPI), a C++23
compiler (GCC 14 or later), CMake, and these libraries:

- [lg](https://github.com/joan2937/lg) (lgpio): download, then `make && sudo make install`
- [Display_Lib_RPI](https://github.com/gavinlyonsrepo/Display_Lib_RPI) 2.7.0: download, then `make && sudo make install`
- ZeroMQ: `sudo apt install libzmq3-dev cppzmq-dev`

Then build, and optionally run the tests:
```shell
cmake -S . -B build
cmake --build build -j3
ctest --test-dir build
```

The display wiring (chip select, DC and reset pins for each screen), SPI speed and other settings
are in [src/config.h](src/config.h). They are compiled in, so rebuild after changing them.

### Running

```shell
build/AnimatedEyes
```

By default the eyes animate by themselves and listen for commands on port 5562. Some settings
can be changed at run time with environment variables, without rebuilding:

| Variable | Meaning |
|----------|---------|
| `EYES_PERSON_ENDPOINT`, `EYES_LIGHT_ENDPOINT` | Where to get face and light data (off by default) |
| `EYES_COMMAND_ENDPOINT` | Where to listen for commands (default `tcp://*:5562`) |
| `MASTER` | Address of the optional [Master](master/README.md) name service, for endpoints given as names |
| `EYES_MIRROR_PERSON_X` | `1` to mirror face positions left to right |
| `EYES_LIGHT_MIN`, `EYES_LIGHT_MAX`, `EYES_LIGHT_CURVE` | How light levels map to pupil size |
| `EYES_STYLE_DURATION_MS` | How long each style is shown when cycling through them |

### Sensors and Commands

The eyes can follow people's faces and react to the ambient light level, using data
fetched over ZeroMQ from a sensor provider on the same or another machine. See
[docs/sensor-protocol.md](docs/sensor-protocol.md) for how to connect or write a provider.

Other programs can send the eyes commands over ZeroMQ, to blink, wink, open or close them,
change how often they blink, or change eye style. See [docs/command-protocol.md](docs/command-protocol.md),
or try it from the command line with [tools/send_command.py](tools/send_command.py).
C++ programs can use the optional header-only API in [api/](api/README.md) instead, e.g.
`animatedeyes::blink()`, without dealing with ZeroMQ directly.

### What does it Look Like?
Here's a video of the eyes in action:
<br/>

[![Teensy Eyes](http://img.youtube.com/vi/Ke1SJ8-6zJw/0.jpg)](https://www.youtube.com/watch?v=Ke1SJ8-6zJw "Teensy Eyes")

(Video of the original TeensyEyes by Chris Miller.)

### 3D Printed Enclosure for the GC9A01 Screens

Chris Miller has made some cases for the GC9A01 screens that you can print. More details and the links to the models are available [here](https://forum.pjrc.com/index.php?threads/uncanny-eyes-is-getting-expensive.71068/page-5#post-333711).

### Creating Your Own Animated Eyes
Each type of eye is defined with a config.eye JSON file specifying various parameters of the eye, pupil, iris and
sclera, plus (optional) bitmaps that define the extents of the upper and lower eyelids, along with any textures for the
iris and sclera.

Detailed documentation for config.eye is coming, but in the meantime, if you want to try creating your own eyes
then copying and editing existing eyes in the `resources/eyes/240x240` directory is a good
starting point. Note that colors are currently only able to be specified as 5:6:5 RGB decimal
or hex values. Try [this](http://greekgeeks.net/#maker-tools_convertColor) for help in creating
your own 5:6:5 color codes.

Once you have an eye definition that you want to try, you need to generate code from it for
use in AnimatedEyes. For this you will need a recent version of [Python](https://www.python.org/) installed, as well as [numpy](https://numpy.org/)
and the imaging library [Pillow](https://python-pillow.org/). You can install these two libraries with:
```shell
pip install numpy
pip install Pillow
```

To generate the data files for a single eye, run the following from the `resources/eyes/240x240` directory:
```shell
python tablegen.py ../../../src/eyes/240x240 path/to/your/config.eye
```
To generate _all_ eyes, run the `genall.py` command shown below. Note that you may want to make sure the output directory is empty first, so you don't end
up with redundant data files in there if your eye's configurations have changed.
```shell
python genall.py ../../../src/eyes/240x240
```

To use your newly created eye, add `#include "eyes/240x240/eyename.h"` to [src/config.h](src/config.h),
and a line for it in `eyeDefinitions`: `{eyename::eye, eyename::eye}`, or `{eyename::left, eyename::right}`
if your eye has different parameters for the left and right eyes. If the generator created any new
`.cpp` table files, add them to `CMakeLists.txt`.

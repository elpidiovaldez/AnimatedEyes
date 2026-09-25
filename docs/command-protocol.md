# Command protocol

Other programs can control AnimatedEyes by sending it text commands over
[ZeroMQ](https://zeromq.org/), for example to make it blink, wink or close its
eyes. Sensor data (faces and light) works the other way round: see
[sensor-protocol.md](sensor-protocol.md).

## Connecting

AnimatedEyes **binds a `PULL` socket**. Senders connect a `PUSH` socket to it and
send one command per message. Any number of senders can connect at once. No
reply is sent; problems are reported in the AnimatedEyes log.

Where AnimatedEyes listens is set by `COMMAND_ENDPOINT` in `src/config.h` (default
`tcp://*:5562`), which the environment variable `EYES_COMMAND_ENDPOINT` overrides. The
value is either:

- a ZeroMQ address, such as `tcp://*:5562` (listen on port 5562 on all network
  interfaces), or
- an endpoint name, such as `eyes.commands`, which is looked up with the
  [Master](../master/README.md) name service at the address in `$MASTER`.

An empty value disables commands.

[`tools/send_command.py`](../tools/send_command.py) sends commands from the
command line. It finds the eyes by looking up `eyes.commands` with the Master (or
uses `EYES_COMMAND_ENDPOINT` if set), and says so if the eyes aren't listening:

```shell
python3 tools/send_command.py blink left
python3 tools/send_command.py "blink_rate 0; open 0"      # several commands
```

When sending from your own code, set a `LINGER` time on the `PUSH` socket so that
a program that exits straight after sending doesn't block forever when the eyes
aren't running.

## Commands

A command is its name followed by arguments separated by spaces. `left`, `right`
or `both` (the default) chooses which eyes a command applies to, and can go
anywhere after the name. "Left" is display 0 in `src/config.h`.

### `blink`

```
blink [left|right|both]
```

Blinks both eyes, or winks one of them. A blink closes the eyelids from wherever
they are (see `open`) and returns them there. An eye that is already blinking
ignores the command.

### `open`

```
open <level> [<seconds>] [left|right|both]
```

Moves the eyelids to `level`, from 0 (fully closed) to 1 (fully open, the
normal state). They stay there, apart from blinks, for `seconds` and then
return to fully open; without `seconds` they stay until the next `open`.
For example:

```
open 0                # close both eyes
open 0.4 left         # squint with the left eye
open 0 2 right        # hold a wink with the right eye for 2 seconds
open 1                # open both eyes fully
```

### `blink_rate`

```
blink_rate [<per minute>]
```

Sets the average number of automatic blinks per minute; the time between
blinks varies randomly around that. Without a number, the rate is set to
`DEFAULT_BLINK_RATE` in `src/config.h` (10), which is also the rate the eyes
start with. `blink_rate 0` turns automatic blinking off, which is useful when the
eyes are closed.

### `style`

```
style <name>|cycle
```

Switches to the named eye style, e.g. `style cat`, and stays on it. `style cycle`
goes back to changing style every `STYLE_DURATION_MS` (10 seconds), which is what
the eyes do when they start. The available styles are the ones included in
`eyeDefinitions` in `src/config.h`; their names are printed in the log at startup,
and again if an unknown name is given.

## Errors

Unknown commands, and commands with missing or invalid fields, are logged and
ignored.

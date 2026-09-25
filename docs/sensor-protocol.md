# Sensor protocol

AnimatedEyes can follow people's faces and react to the ambient light level. It gets
this data by asking another program, the **sensor provider**, over
[ZeroMQ](https://zeromq.org/). A provider can run on the same machine or anywhere on
the network, and can be written in any language that has ZeroMQ bindings.

A provider can serve faces, light, or both. The eyes still animate on their own when
no provider is configured or running, and pick the data up automatically when one
appears.

## Connecting

The provider **binds a `REP` socket** (for example `tcp://*:5561`). AnimatedEyes
connects a `REQ` socket to it, sends a request roughly every 100 ms, and waits
up to 500 ms for each reply.

AnimatedEyes is told where to connect by two settings in `src/config.h`, which the
environment variables below override:

| Setting | Environment variable | Used for |
|---------|----------------------|----------|
| `PERSON_ENDPOINT` (default off) | `EYES_PERSON_ENDPOINT` | `faces` requests |
| `LIGHT_ENDPOINT` (default off) | `EYES_LIGHT_ENDPOINT` | `light` requests |

Each value is either:

- a ZeroMQ address, such as `tcp://192.168.1.120:5561`, or
- an endpoint name, such as `eyes.sensors`, which is looked up with the
  [Master](../master/README.md) name service at the address in `$MASTER`.

An empty value disables that sensor. For example:

```shell
EYES_PERSON_ENDPOINT=tcp://camerapi:5561 EYES_LIGHT_ENDPOINT= ./AnimatedEyes
```

## Messages

Requests and replies are single UTF-8 text messages. Reply lines hold
`key=value` fields separated by spaces; values cannot contain spaces. Unknown
keys are ignored, so providers can add their own.

### `faces`

Reply: a header line, then one line per face detected (none if no faces).

```
faces age_ms=40
x=0.512 y=0.430 w=0.120 h=0.160 conf=0.93 id=paul
x=0.210 y=0.550 w=0.080 h=0.110 conf=0.61
```

| Key | Required | Meaning |
|-----|----------|---------|
| `age_ms` (header) | no | How old the data is, in milliseconds. Data over 1000 ms old is treated as "no faces". |
| `x`, `y` | yes | Centre of the face as a fraction of the image: `0,0` is the top-left corner and `1,1` the bottom-right. |
| `w`, `h` | no | Face width and height as fractions of the image. The eyes follow the largest face. |
| `conf` | no | Detection confidence, 0 to 1 (default 1). Faces below `MIN_FACE_CONFIDENCE` are ignored. |
| `facing` | no | `1` if the person faces the camera, `0` if not (default 1). Faces not facing are ignored. |
| `id` | no | Name or ID of a recognised person. |

Coordinates are as seen by the camera. If the camera faces outwards like the
eyes do, leave `MIRROR_PERSON_X` set to `true` in `src/config.h`.

### `light`

```
light level=0.42 age_ms=40
```

| Key | Required | Meaning |
|-----|----------|---------|
| `level` | yes | Ambient light, from 0 (dark) to 1 (bright). Bright light makes the pupils smaller. |
| `age_ms` | no | How old the reading is, in milliseconds. Readings over 2000 ms old are ignored. |

### Errors

A provider that can't answer a request replies with a line starting with `error`,
for example `error unknown request 'light'`. AnimatedEyes logs it and carries on.

## Writing a provider

[`tools/example_sensor_provider.py`](../tools/example_sensor_provider.py) is a
complete provider in about 40 lines. It serves a pretend face moving in a circle
and a changing light level, which is also handy for testing the eyes without a
camera.

A provider must reply to every request it receives, because a `REP` socket
accepts nothing new until it has replied. Answer quickly from the latest
results, rather than waiting for the next camera frame.

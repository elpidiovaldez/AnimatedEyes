# Setup using Master

This page shows how to run AnimatedEyes, a sensor provider and command senders on
different machines, using the [Master](../master/README.md) name service so that no
program needs host names or port numbers built in. Using the Master is optional:
every endpoint can instead be given directly as a ZeroMQ address (see
[sensor-protocol.md](sensor-protocol.md) and [command-protocol.md](command-protocol.md)).

In the examples below:

- the Master runs on `clio`, port 6000,
- the face sensor provider also runs on `clio`, and
- AnimatedEyes runs on `halie`.

Replace these with your own host names, or with IP addresses if the machines can't
resolve each other's names.

## 1. List the endpoints in the Master's YAML file

```yaml
eyes:
  sensors: tcp://clio:5561     # the machine running the sensor provider
  commands: tcp://halie:5562   # the machine running AnimatedEyes
```

Endpoint names are the flattened keys, e.g. `eyes.sensors`. Each address names the
machine that **binds** the socket: that program listens on the given port on all its
network interfaces, and every other program connects to the address as written.

Start the Master on `clio`:

```shell
build/master/master /path/to/endpoints.yaml
```

## 2. Start the sensor provider

The provider binds its `REP` socket to `eyes.sensors`. With the Python client in
[master/python/master_client.py](../master/python/master_client.py):

```python
import zmq
from master_client import master

eyesSocket = master.bind("eyes.sensors", zmq.REP)   # listens on tcp://*:5561
```

Run it with `MASTER` set, on `clio`:

```shell
export MASTER=tcp://clio:6000
python3 my_face_provider.py
```

## 3. Start AnimatedEyes

On `halie`, export the Master's address and give the endpoints by name:

```shell
export MASTER=tcp://clio:6000
export EYES_PERSON_ENDPOINT=eyes.sensors     # faces
export EYES_LIGHT_ENDPOINT=eyes.sensors      # light level, if the provider serves it
export EYES_COMMAND_ENDPOINT=eyes.commands   # listen for commands where the Master says
build/AnimatedEyes
```

`EYES_PERSON_ENDPOINT` and `EYES_LIGHT_ENDPOINT` are off by default, so the eyes won't
follow faces unless `EYES_PERSON_ENDPOINT` is set. `EYES_COMMAND_ENDPOINT` defaults to
`tcp://*:5562`, which works too, but using the name keeps the port in the YAML file.

To make these settings permanent, put the `export` lines in the script that starts
AnimatedEyes. Variables exported in your login shell don't reach:

- `sudo`, which clears the environment. Use `sudo -E build/AnimatedEyes`, or
  `sudo MASTER=tcp://clio:6000 EYES_PERSON_ENDPOINT=eyes.sensors build/AnimatedEyes`.
- a systemd service. Set them in the unit file instead:

  ```ini
  [Service]
  Environment=MASTER=tcp://clio:6000
  Environment=EYES_PERSON_ENDPOINT=eyes.sensors
  Environment=EYES_LIGHT_ENDPOINT=eyes.sensors
  Environment=EYES_COMMAND_ENDPOINT=eyes.commands
  ```

### Checking the connection

AnimatedEyes logs the state of each link. When everything works you'll see:

```
Sensor 'faces' (eyes.sensors): connecting to tcp://clio:5561
Sensor 'faces' (eyes.sensors): receiving data
```

| Log | Meaning |
|-----|---------|
| No `Sensor 'faces'` lines | `EYES_PERSON_ENDPOINT` is empty, so face tracking is off. |
| `MASTER environment variable not set` | `MASTER` didn't reach AnimatedEyes (see above). |
| A lookup error or timeout | The Master isn't running, isn't reachable at `$MASTER`, or doesn't know the name. |
| `no reply` / `waiting for provider` | The provider isn't running, or `halie` can't reach `clio:5561`. |

You can query the Master and the provider by hand from any machine:

```python
import zmq

context = zmq.Context()

def ask(address, request):
    socket = context.socket(zmq.REQ)
    socket.setsockopt(zmq.RCVTIMEO, 3000)
    socket.setsockopt(zmq.LINGER, 0)
    socket.connect(address)
    socket.send_string(request)
    return socket.recv_string()   # raises zmq.Again after 3 seconds without a reply

sensors = ask("tcp://clio:6000", "eyes.sensors")   # -> 'tcp://clio:5561'
print(ask(sensors, "faces"))                        # -> 'faces age_ms=42\nx=0.51 y=0.43 ...'
```

## 4. Send commands from Python

From the command line, [tools/send_command.py](../tools/send_command.py) looks up
`eyes.commands` with the Master:

```shell
export MASTER=tcp://clio:6000
python3 tools/send_command.py blink left
python3 tools/send_command.py "blink_rate 0; open 0"
```

From your own program, connect a `PUSH` socket to `eyes.commands` and send one
command per message:

```python
import zmq
from master_client import master   # master/python/master_client.py; needs $MASTER

eyes = master.connect("eyes.commands", zmq.PUSH)
# Give unsent commands up to 2 seconds to go when the program exits, rather than
# discarding them (the client's default) or blocking forever if the eyes aren't running.
eyes.setsockopt(zmq.LINGER, 2000)

eyes.send_string("blink")             # blink both eyes
eyes.send_string("open 0.4 3 left")   # squint with the left eye for 3 seconds
eyes.send_string("style cat")         # switch to the cat eye style
```

The commands are listed in [command-protocol.md](command-protocol.md#commands). No reply
is sent; problems are reported in the AnimatedEyes log. Note that
`master_client.py` waits for the Master without a timeout, so a program using it hangs
at `master.connect` if the Master isn't running.

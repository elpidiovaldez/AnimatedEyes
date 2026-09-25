# Master (endpoint name service)

A tiny ZeroMQ name service, taken from the BDX robot project. Processes ask the
Master for an endpoint by name (e.g. `eyes.sensors`) instead of hard-coding
host/port addresses, so the whole system is reconfigured by editing one YAML file.

Using the Master is **optional** for AnimatedEyes: sensor endpoints can also be
given directly as ZeroMQ addresses (see [docs/sensor-protocol.md](../docs/sensor-protocol.md)).

| Directory | Contents |
|-----------|----------|
| `server/` | `master` executable. Loads an endpoints YAML file and answers lookups. Needs yaml-cpp. |
| `client/` | C++ `Master` client (used by AnimatedEyes). |
| `python/` | Python `Master` client, for providers written in Python. Not used by AnimatedEyes itself; a copy of BDX's `bdx_python/master_client.py` for reference. |

## Protocol

REQ/REP on the address in the `MASTER` environment variable (e.g. `tcp://clio:6000`).
The request is an endpoint name; the reply is its address, or a string starting
with `ERROR:` if the name is unknown.

## Running the server

```shell
sudo apt install libyaml-cpp-dev libzmq3-dev cppzmq-dev
cmake -S master/server -B build/master && cmake --build build/master
export MASTER=tcp://clio:6000
build/master/master /path/to/endpoints.yaml
```

A relative YAML path is resolved from the directory containing the `master`
executable. See `server/example-endpoints.yaml`.

## Differences from the BDX copy

`client/` adds a lookup timeout (`Master::LOOKUP_TIMEOUT_MS`) so a missing Master
throws instead of blocking forever.

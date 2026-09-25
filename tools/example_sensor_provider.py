#!/usr/bin/env python3
"""Example AnimatedEyes sensor provider (see docs/sensor-protocol.md).

Serves a pretend face that moves slowly in a circle, and a light level that slowly
rises and falls. Use it to test AnimatedEyes without a camera, or as a starting point
for your own provider:

    pip install pyzmq
    python3 example_sensor_provider.py --bind tcp://*:5561

then run AnimatedEyes with EYES_PERSON_ENDPOINT=tcp://<this host>:5561 (and the same
for EYES_LIGHT_ENDPOINT).
"""

import argparse
import math
import time

import zmq

parser = argparse.ArgumentParser()
parser.add_argument("--bind", default="tcp://*:5561", help="ZeroMQ address to serve requests on")
args = parser.parse_args()

socket = zmq.Context.instance().socket(zmq.REP)
socket.bind(args.bind)
print(f"Serving AnimatedEyes requests on {args.bind}")

start = time.monotonic()

while True:
    request = socket.recv_string()
    t = time.monotonic() - start

    if request == "faces":
        x = 0.5 + 0.3 * math.cos(t / 3)
        y = 0.5 + 0.2 * math.sin(t / 3)
        reply = f"faces age_ms=0\nx={x:.3f} y={y:.3f} w=0.15 h=0.2 conf=0.9 id=example"
    elif request == "light":
        level = 0.5 + 0.5 * math.sin(t / 5)
        reply = f"light level={level:.3f} age_ms=0"
    else:
        reply = f"error unknown request '{request}'"

    socket.send_string(reply)

#!/usr/bin/env python3
"""Sends a command to AnimatedEyes (see docs/command-protocol.md).

    send_command.py blink left
    send_command.py open 0.3 5
    send_command.py "blink_rate 0; open 0"      (several commands, separated by ';')

AnimatedEyes is found by looking up "eyes.commands" with the Master name service at
$MASTER. Set EYES_COMMAND_ENDPOINT to a ZeroMQ address (e.g. tcp://asia:5562) to
connect directly instead, or to a different Master endpoint name.
"""

import os
import sys

import zmq

TIMEOUT_MS = 2000

if len(sys.argv) < 2:
    sys.exit(__doc__)

context = zmq.Context.instance()


def lookup(name):
    """Asks the Master service at $MASTER for the address of endpoint `name`."""
    master_address = os.environ.get("MASTER")
    if master_address is None:
        sys.exit("MASTER environment variable not set (or set EYES_COMMAND_ENDPOINT to an address)")
    socket = context.socket(zmq.REQ)
    socket.setsockopt(zmq.LINGER, 0)
    socket.setsockopt(zmq.RCVTIMEO, TIMEOUT_MS)
    socket.connect(master_address)
    socket.send_string(name)
    try:
        reply = socket.recv_string()
    except zmq.Again:
        sys.exit(f"No reply from Master at {master_address}")
    if reply.startswith("ERROR"):
        sys.exit(f"Master at {master_address} doesn't know '{name}': {reply}")
    return reply


endpoint = os.environ.get("EYES_COMMAND_ENDPOINT", "eyes.commands")
address = endpoint if "://" in endpoint else lookup(endpoint)
address = address.replace("*", "localhost")   # A bind address like tcp://*:5562 means this machine

# Only send once actually connected, so we can tell if the eyes aren't listening
socket = context.socket(zmq.PUSH)
socket.setsockopt(zmq.IMMEDIATE, 1)
socket.setsockopt(zmq.SNDTIMEO, TIMEOUT_MS)
socket.setsockopt(zmq.LINGER, TIMEOUT_MS)
socket.connect(address)

for command in " ".join(sys.argv[1:]).split(";"):
    command = command.strip()
    if command:
        try:
            socket.send_string(command)
        except zmq.Again:
            sys.exit(f"AnimatedEyes is not listening at {address}")

socket.close()

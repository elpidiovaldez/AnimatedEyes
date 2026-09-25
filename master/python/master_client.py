import os
import zmq

class Master:
    def __init__(self):
        self.context = zmq.Context.instance()

        self.socket = self.context.socket(zmq.REQ)
        self.socket.setsockopt(zmq.LINGER, 0)

        address = os.environ.get("MASTER")
        if address is None:
            raise RuntimeError("MASTER environment variable not set")

        self.socket.connect(address)

    def lookup(self, name):
        self.socket.send_string(name)
        response = self.socket.recv_string()

        if response.startswith("ERROR:"):
            raise RuntimeError(response)

        return response

    def transform_to_bind_address(self, address):
        if address.startswith("tcp://"):
            port_start = address.find(":", 6)

            if port_start == -1:
                raise RuntimeError("TCP endpoint has no port")

            return "tcp://*" + address[port_start:]

        if address.startswith(("inproc://", "ipc://")):
            return address

        raise RuntimeError("Unsupported ZeroMQ transport")

    def connect(self, name, socket_type):
        socket = self.context.socket(socket_type)
        socket.setsockopt(zmq.LINGER, 0)
        socket.connect(self.lookup(name))
        return socket

    def bind(self, name, socket_type):
        socket = self.context.socket(socket_type)
        socket.setsockopt(zmq.LINGER, 0)
        socket.bind(
            self.transform_to_bind_address(self.lookup(name))
        )
        return socket
        
master = Master()


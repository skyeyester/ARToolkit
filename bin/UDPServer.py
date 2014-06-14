try:#Python 2.X
	from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
	from SocketServer import ThreadingUDPServer, DatagramRequestHandler, \
                             ThreadingTCPServer, StreamRequestHandler
except ImportError:#Python 3.X
	from http.server import HTTPServer, BaseHTTPRequestHandler
	from socketserver import ThreadingUDPServer, DatagramRequestHandler, \
                             ThreadingTCPServer, StreamRequestHandler
import threading
import time
import socket
import SocketServer

import os
from PNG2RAW import PNG2RAW
import subprocess


class ControlMixin(object):
	def __init__(self, handler, poll_interval):
		self._thread = None
		self.poll_interval = poll_interval
		self._handler = handler
 
	def start(self):
		# Start a thread with the server -- that thread will then start one
		# more thread for each request
		self._thread = t = threading.Thread(target=self.serve_forever,
                                            args=(self.poll_interval,))
		# Exit the server thread when the main thread terminates
		t.setDaemon(True)
		t.start()
		#print "Server loop running in thread:", self._thread.name

	def stop(self):
		self.shutdown()
		self._thread.join()
		self._thread = None

class ThreadedUDPServer(ControlMixin, ThreadingUDPServer):
	def __init__(self, addr, handler, poll_interval=0.5, bind_and_activate=True):
		class DelegatingUDPRequestHandler(DatagramRequestHandler):
 
			def handle(self):
				self.server._handler(self)
		ThreadingUDPServer.__init__(self, addr, DelegatingUDPRequestHandler,
                                    bind_and_activate)
		ControlMixin.__init__(self, handler, poll_interval)
		
def client(ip, port, message):
	# SOCK_DGRAM is the socket type to use for UDP sockets
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	
	# As you can see, there is no connect() call; UDP has no connections.
	# Instead, data is directly sent to the recipient via sendto().

	try:
		sock.sendto(message + "\n", (ip, port))
		received = sock.recv(1024)
		print "Sent: "
		print message
		print "Received: "
		print received
	finally:
		sock.close()

def monitor(notify):
		
		"""for debug
		data = notify.request[0].strip()
		socket = notify.request[1]
		print notify.client_address[0]+" wrote: "
		print data
		socket.sendto(data.upper()+"\n", notify.client_address)
		"""

		"""
		data = ''
		while 1:
			c = notify.request.recv(1)
			if c == '\n' or c == '':
				break
			else:
			  data += c
			if len(data.split('=')) is 2:
				size = int(data.split('=').pop())
			else:
				return
			s = size
			data = ''
			while size:
				c = notify.request.recv(1)
				if c == '':
					continue
				data += c
				size -= 1
		convertor = PNG2RAW(data)
		outbuf = convertor.convert()
		os.write(notify.server.pin, 'size=%d\r\n %s' % (convertor.getLen(), outbuf))
		"""
		data = notify.request[0]
		print "---------------------------------------------------------------"
		print data
		convertor = PNG2RAW(data)
		outbuf = convertor.convert()
		os.write(notify.server.pin, 'size=%d\r\n %s' % (convertor.getLen(), outbuf))


if __name__ == "__main__":
	# Port 0 means to select an arbitrary unused port
	HOST, PORT = "192.168.11.4", 5006
	udpserver = ThreadedUDPServer((HOST, PORT), monitor, 0.001)

	pipein, pipeout = os.pipe()
	udpserver.pin = pipeout
	artoolkit_proc = subprocess.Popen(['/home/hscc/MyARProject/ARToolKit/bin/loadMultiple'], stdin=pipein)
	
	udpserver.start()

	""" for debug
	ip, port = udpserver.server_address 
	client(ip, port, "Hello World 1")
	client(ip, port, "Hello World 2")
	client(ip, port, "Hello World 3")
	"""
	udpserver.stop()

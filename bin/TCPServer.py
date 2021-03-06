import socket
import threading
import SocketServer
import time
import os
from PNG2RAW import PNG2RAW
import subprocess

class OutputThread(threading.Thread):
	
	def __init__(self, connection, process_out):
		readFD = os.fdopen(os.dup(process_out))
		#assert callable(readFD.readline)
		threading.Thread.__init__(self)
		self.socket=connection
		print 'BBB\n'
		print process_out
		print connection
		print readFD
		print '\n'
		self.pipefd=readFD

	def run(self):
		#read from pipe
		print 'CCC\n'
		print self.pipefd
		print self.socket
		print '\n'   
		#pipein = os.fdopen(self.pipefd)
		#line=pipein.readiline()
		#line = os.read(self.pipefd, 128)
		"""
		for line in iter(self.pipefd.readline(), ''):
		#send result to client
		result="in Thread:"+line+"\r\n"
		#self.socket.sendall(result)
		print result
        """
		line = self.pipefd.readline()
		result="in Thread:"+line+"\n"
		print result
		#self.pipefd.close()
		#self.socket.sendall(result)
			

class ThreadedTCPRequestHandler(SocketServer.BaseRequestHandler):

    def handle(self):
		data = ''
		while 1:
			c = self.request.recv(1)
			if c == '\n' or c == '':
				break
			else:
				data += c

		if len(data.split('=')) is 2:
			size = int(data.split('=').pop())
		else:
			return

		#print size
		s = size
		data = ''
		while size:
			c = self.request.recv(1)
			if c == '':
				continue
			data += c
			size -= 1
		
		#print len(data)
		convertor = PNG2RAW(data)
		outbuf = convertor.convert()
		#print convertor.getSize()
		#print convertor.getLen()
		os.write(self.server.pin, 'size=%d\r\n %s' % (convertor.getLen(), outbuf))
		#os.write(self.server.pin, outbuf)
		
		"""
		create thread deal with stdout
		"""
		#print 'AAA\n'
		#print self.server.pout
		#print self.request
		#print '\n'
		#outThread = OutputThread(self.request, self.server.pout)
		#outThread.start()
		#response = "Hello: {}\r\n"
		#self.request.sendall(response)
		#self.request.close()      
	    
		"""			
		print self.server.test
		print self.request.fileno()
		from subprocess import call
		call(['/home/ctchang/src/ARToolKit/bin/simpleTest'], stdin=self.request.fileno())
		data = self.request.recv(1024)
        cur_thread = threading.current_thread()
        response = "{}: {}".format(cur_thread.name, data)
        self.request.sendall(response)
		time.sleep(5)
		self.request.sendall('over'+cur_thread.name)
		"""


class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
	pass

if __name__ == "__main__":
    # Port 0 means to select an arbitrary unused port
	HOST, PORT = "192.168.11.5", 5006
	#HOST, PORT = "localhost", 5005 
	#HOST, PORT = "172.18.175.73", 5006
	
	
	server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
    
	ip, port = server.server_address
	print ip
	print port
	
	pipein, pipeout = os.pipe()
	pipein2, pipeout2 = os.pipe()

	server.pin = pipeout
	#server.pout = pipein2
	#artoolkit_proc = subprocess.Popen(['/home/hscc/MyARProject/ARToolKit/bin/simpleTest'], stdin=pipein)
	artoolkit_proc = subprocess.Popen(['/home/hscc/MyARProject/ARToolKit/bin/loadMultiple'], stdin=pipein) 

	# Start a thread with the server -- that thread will then start one
    # more thread for each request
	print "Server loop running in thread1:"
	server_thread = threading.Thread(target=server.serve_forever())
	print "Server loop running in thread2:", server_thread.name
    # Exit the server thread when the main thread terminates

	server_thread.daemon = True
	print "Server loop running in thread3:", server_thread.name
	server_thread.start()
	print "Server loop running in thread:", server_thread.name

	time.sleep(100)
	#client(ip, port, "Hello World 1")
    #client(ip, port, "Hello World 2")
    #client(ip, port, "Hello World 3")

#    server.shutdown()


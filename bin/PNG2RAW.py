from PIL import Image
import StringIO

class PNG2RAW:
	"""
	This class is for translating PNG image in RGBA to RAW image in RGB.

	Usage:
		PNG2RAW obj(srcBuf)
		dstBuf = obj.convert()
	"""

	def __init__(self, srcBuf):
		self.readImg(srcBuf)

	def readImg(self, srcBuf):
		self.srcBuffer = srcBuf
		strio = StringIO.StringIO(self.srcBuffer)
		self.img = Image.open(strio).convert('RGB')

	def convert(self):
		"""
		Returns:
			A string: raw data of img in RGB.
		"""
		if self.img is None:
			self.readImg()

		outBuf = ''
		for r, g, b in list(self.img.getdata()):
			#r, g, b are integer
		    outBuf += '%c%c%c' % (r, g, b)
		return outBuf


	def getLen(self):
		x, y = self.img.size
		return x*y*3


	def getSize(self):
		"""
		Returns:
			<x, y>
		"""
		return self.img.size

	def getMode(self):
		"""
		Returns:
			A string: ex. RGB
		"""
		return self.img.mode

   
if __name__ == "__main__":

	f = open('sample.png', 'rb')
	buf = f.read()
	
	convertor = PNG2RAW(buf)
	print 'create object'
	print convertor.getMode()
	print convertor.getSize()

	outBuf = convertor.convert()
	print len(outBuf)
	print convertor.getLen()


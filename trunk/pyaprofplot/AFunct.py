import numpy as np
from operator import itemgetter

class AFunct: 
	def __init__(self, rName, imName, id): 
		self.rName=rName
		self.imName=imName
		self.id=id
		self.rmsSet=np.array([[0,0,0,0,0,0,0,0,0,0,0]])
		self.totCount=0
		self.Nrms=0;
	def addRms(self, rms):
		rms=map(int,rms[2:])
		self.rmsSet=np.vstack([self.rmsSet,rms])
		self.totCount+=int(rms[7])
		self.Nrms+=1
	def getN(self):
		self.rName
	def reorder(self):
		temp=self.rmsSet[:,0].argsort()
		self.rmsSet=self.rmsSet[temp]


import numpy as np
from operator import itemgetter

class AFunct: 
	def __init__(self, rName, imName, fid,libName): 
		self.rName=rName
		self.imName=imName
		self.fid=fid
		self.rmsSet=np.array([[0,0,0,0,0,0,0,0,0,0,0]])
		self.totCalls=0
		self.totCallsPerc=0
		self.cumulativeCost=0
		self.cumulativeCostPerc=0
		self.Nrms=0
		self.lib=libName
	def addRms(self, rms):
		rms=map(int,rms[2:])
		self.rmsSet=np.vstack([self.rmsSet,rms])
		self.totCalls+=rms[5]
		self.cumulativeCost+=rms[6]
		self.Nrms+=1
	def getN(self):
		self.rName
	def reorder(self):
		temp=self.rmsSet[:,0].argsort()
		self.rmsSet=self.rmsSet[temp]



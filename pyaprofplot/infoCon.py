class infoCon:
	def __init__(self,vN,k,ex,m,tCalls):
		self.versionNumber=vN
		self.totalCumulCost=k
		self.execName=ex
		self.metric=m
		self.functContainer=[]
		self.totFunctsCalls=tCalls
	def addFunct(self,funct):
		self.functContainer.append(funct)
		funct.totCallsPerc=round(100*float(funct.totCalls)/self.totFunctsCalls,2)
		funct.cumulativeCostPerc=round(100*float(funct.cumulativeCost)/self.totalCumulCost,2)

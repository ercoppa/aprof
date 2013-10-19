class infoCon:
	def __init__(self,vN,k,ex,m,tC):
		self.versionNumber=vN
		self.totalCost=k
		self.execName=ex
		self.metric=m
		self.functContainer=[]
		self.totCount=tC	
	def addFunct(self,funct):
		self.functContainer.append(funct)
		

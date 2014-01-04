import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import pylab
from matplotlib import colors
import numpy as np
import sys as sys
import os
import math

cmap = plt.cm.jet
bounds=[-1,0,1,4,8,12,16,32,128,256,1024,2048,4000,sys.maxint-1,sys.maxint]
norm = colors.BoundaryNorm(bounds, cmap.N)
newBounds=[1,4,8,12,16,32,128,256,1024,1800,6500]

def costPlot(funct):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,2])
	f=np.append(f,funct.rmsSet[:,5])
	pylab.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(y)+(10*np.amax(y)/100)])
	pylab.xlabel('read memory size',fontsize=15)
	pylab.ylabel("cost",fontsize=15)
	pylab.grid(True)
	sc=pylab.scatter(x[1:],y[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[1, 16,256,4000],aspect=20,shrink=0.4)
	cbar.set_label("RMS frequency",size=15)
	"""plt.show()"""
	fig=file(os.path.join("tmp",funct.rName+'_cost.eps'),'wb')
	pylab.savefig(fig,format='eps')
	fig.close()
	pylab.close()
	return "cost"
	
def RMSFreqPlot(funct):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(f,funct.rmsSet[:,5])
	pylab.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(y)+(10*np.amax(y)/100)])
	pylab.xlabel("read memory size",fontsize=15)
	pylab.ylabel("frequency",fontsize=15)
	pylab.grid(True)
	sc=pylab.scatter(x[1:],y[1:],s=6, marker = 's',lw=0.0)
	fig=file(os.path.join("tmp",funct.rName+'_RMSFreq.eps'),'wb')
	pylab.savefig(fig,format='eps')
	fig.close()
	pylab.close()
	return "RMSFreq"
	
def TotalCostPlot(funct):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	pylab.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(y)+(10*np.amax(y)/100)])
	pylab.xlabel("read memory size",fontsize=15)
	pylab.ylabel("cost",fontsize=15)
	pylab.grid(True)
	sc=pylab.scatter(x[1:],y[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[1, 16,256,4000],aspect=20,shrink=0.4)
	cbar.set_label("RMS frequency",size=15)
	"""plt.show()"""
	fig=file(os.path.join("tmp",funct.rName+'_TotCost.eps'),'wb')
	pylab.savefig(fig,format='eps')
	fig.close()
	pylab.close()
	return "TotCost"
	
def curveBounding(funct,B):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,2])
	f=np.append(f,funct.rmsSet[:,5])
	z=np.array([])
	z=np.append(z,[0])
	i=1
	while i<len(x):
		den=x[i]*math.log(x[i])
		if den>0:
			z=np.append(z,[y[i]/den])
		else:
			 z=np.append(z,[0])
		i+=1
	pylab.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(z)+(10*np.amax(z)/100)])
	pylab.xlabel('read memory size',fontsize=15)
	pylab.ylabel("ratio",fontsize=15)
	pylab.grid(True)
	sc=pylab.scatter(x[1:],z[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[1, 16,256,4000],aspect=20,shrink=0.4)
	cbar.set_label("RMS frequency",fontsize=15)
	pylab.title("T(n)/nlog(n)",fontsize=20)	
	fig=file(os.path.join("tmp",funct.rName+'_curve_Bounding.eps'),'wb')
	pylab.savefig(fig,format='eps')
	fig.close()
	pylab.close()
	return "curve_Bounding"

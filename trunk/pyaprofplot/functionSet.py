import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import pylab
from matplotlib import colors
import numpy as np
import sys as sys
import os
import math
import fitting as fit
import matplotlib.gridspec as gridspec

cmap = plt.cm.jet
bounds=[-1,0,1,4,8,12,16,32,128,256,1024,2048,4000,sys.maxint-1,sys.maxint]
norm = colors.BoundaryNorm(bounds, cmap.N)
newBounds=[1,4,8,12,16,32,128,256,1024,1800,6500]

def create_grid(f,functL):
	fig = plt.figure()
	functN=len(functL)
	r=0
	c=0
	v=0
	retval = os.getcwd()
	os.chdir("tmp")
	'''adjusting factor for mam legend'''
	global mamAdjust
	if functN==1 or functN==2:
		mamAdjust=-0.13
	elif functN==3 or functN==4:
		mamAdjust=-0.32
	elif functN==5:
		mamAdjust=-0.50
	''''''
	if functN==1:
		gs = gridspec.GridSpec(1, 2)
	elif functN%2!=0:
		gs = gridspec.GridSpec((functN+1)/2, 2)
	else:
		gs = gridspec.GridSpec(functN/2, 2)
	gs.update(wspace=0.25,hspace=0.5)

	if functN%2!=0:
		globals()[functL[0]](f,fig.add_subplot(gs[0,:]))
		r=1
		v=1
	for i in functL[v:]:
		globals()[i](f,fig.add_subplot(gs[r,c]))
		if c==1:
			c=0
			r+=1
		else:
			c+=1


	fig.savefig(f.rName+".eps",bbox_inches="tight",dpi=400)
	os.chdir(retval)

	
def costPlot(funct,pl):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,2])
	f=np.append(f,funct.rmsSet[:,5])
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.set_xlabel('read memory size',fontsize=9)
	pl.set_ylabel("cost",fontsize=9)
	pl.grid(True)
	pl.set_title("Cost")
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x[1:],y[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	pylab.close()
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[],aspect=20,shrink=0.0,pad=0.00)
	"""plt.show()"""
	
def mamPlot(funct,pl):
	x=np.array([])
	ymin=np.array([])
	yavg=np.array([])
	ymax=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	ymin=np.append(ymin,funct.rmsSet[:,1])
	ymax=np.append(ymax,funct.rmsSet[:,2])
	t1=funct.rmsSet[:,3]
	t2=funct.rmsSet[:,5]
	yavg=np.append(yavg,t1[1:]/t2[1:])
	f=np.append(f,funct.rmsSet[:,5])
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(ymax)+(2*np.amax(ymax)/100)])
	pl.set_xlabel('read memory size',fontsize=9)
	pl.set_ylabel("cost",fontsize=9)
	pl.grid(True)
	pl.set_title("Min/Avg/Max Cost")
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x[1:],ymax[1:],s=7,c='r', marker = 's',lw=0.0)
	sc1=pl.scatter(x[1:],yavg,s=6.5,c='g', marker = 's',lw=0.0)	
	sc2=pl.scatter(x[1:],ymin[1:],s=6,c='b', marker = 's',lw=0.0)	
	pl.legend((sc2,sc1,sc),("Min","Avg","Max"),scatterpoints=1,ncol=3,bbox_to_anchor=[0.5, mamAdjust],loc="lower center",fontsize=8)
	
def RMSFreqPlot(funct,pl):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(f,funct.rmsSet[:,5])
	pl.axis([-2, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.set_xlabel("read memory size",fontsize=9)
	pl.set_ylabel("frequency",fontsize=9)
	pl.set_title("RMS Frequency")

	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	pl.grid(True)
	sc=pl.scatter(x[1:],y[1:],s=6, marker = 's',lw=0.0)
	
def TotalCostPlot(funct,pl):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.set_xlabel("read memory size",fontsize=9)
	pl.set_ylabel("cost",fontsize=9)
	pl.grid(True)
	pl.set_title("Total Cost")
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x[1:],y[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	pylab.close()
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[],aspect=20,shrink=0.0,pad=0.00)
	
def curveBounding(funct,B,pl):
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
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(z)+(2*np.amax(z)/100)])
	pl.xlabel('read memory size',fontsize=9)
	pl.ylabel("ratio",fontsize=9)
	pl.grid(True)
	sc=pl.scatter(x[1:],z[1:],s=6,c=f[1:], marker = 's',cmap=cmap,norm=norm,lw=0.0)
	pylab.close()
	cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[1, 16,256,4000],aspect=20,shrink=0.4)
	cbar.set_label("RMS frequency",fontsize=15)
	pl.title("T(n)/nlog(n)",fontsize=20)

def MeanCumulativePlot(funct,pl):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	z=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	z=np.append(z,[0])
	num=0
	den=0
	i=1
	while i<len(x):
		num+=y[i]
		den+=f[i]
		z=np.append(z,[num/den])
		i+=1
	pl.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(z)+(10*np.amax(z)/100)])
	pl.set_xlabel("read memory size",fontsize=9)
	pl.set_ylabel("mean cumulative cost",fontsize=9)
	pl.set_title("Mean Cumulative Cost")
	pl.grid(True)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x[1:],z[1:],s=6,marker = 's',lw=0.0)
	"""plt.show()"""

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import pylab
from matplotlib import colors
import numpy as np
import sys as sys
import os
import math
import matplotlib.gridspec as gridspec
import time
import datetime
import clusterFit as cf
cmap = plt.cm.jet
bounds=[-1,0,1,4,8,12,16,32,128,256,1024,2048,4000,sys.maxint-1,sys.maxint]
norm = colors.BoundaryNorm(bounds, cmap.N)
newBounds=[1,4,8,12,16,32,128,256,1024,1800,6500]
np.seterr(all='call')
def errorhandler(errstr, errflag):
	return [-1,-1,1]
np.seterrcall(errorhandler)
plt.rc('legend',**{'fontsize':10})


def fitParameters(f):
	if f.toFit==False:
		return [f,0,0,0,0,0,f.rName,f.lib,f.Nrms,f.cumulativeCost,f.cumulativeCostPerc,f.totCalls,f.totCallsPerc]
	else:
		result=cf.clusterFit(f)
		if len(result)==0:
			return [f,0,0,0,0,result,f.rName,f.lib,f.Nrms,f.cumulativeCost,f.cumulativeCostPerc,f.totCalls,f.totCallsPerc]
		r0=result[0]
		return [f,r0[0],r0[1],r0[2],r0[3],result,f.rName,f.lib,f.Nrms,f.cumulativeCost,f.cumulativeCostPerc,f.totCalls,f.totCallsPerc]
		

def create_grid(args):
	f=args[0][0]
	fitData=args[0][1:]
	functL=args[1]
	epsMin=args[2]
	autolog=args[3]
	alpha=args[4]
	dat=args[0][5]
	functB=""
	fig = plt.figure()
	functN=len(functL)
	r=0 #row
	c=0 #column
	v=0
	#adjusting factor for mam legend
	global mamAdjust
	if functN==1 or functN==2:
		mamAdjust=-0.16
	elif functN==3 or functN==4:
		mamAdjust=-0.32
	elif functN==5 or functN==6:
		mamAdjust=-0.53
	elif functN==7:
		mamAdjust=-0.78
	''''''
	if functN<=2:
		gs = gridspec.GridSpec(2, 2,height_ratios=[1,0.6])
	elif functN%2!=0:
		if functN==3:
			gs = gridspec.GridSpec((functN+1)/2, 2,height_ratios=[1.2,1,1])
			gs.update(wspace=0.20,hspace=0.3)
		elif functN==4:
			gs = gridspec.GridSpec((functN+1)/2, 2,height_ratios=[1,1,1])
			gs.update(wspace=0.15,hspace=0.3)
		elif functN<7:
			gs = gridspec.GridSpec((functN+1)/2, 2,height_ratios=[1.2,1,1])
			gs.update(wspace=0.20,hspace=0.55)
		else:
			gs = gridspec.GridSpec((functN+1)/2, 2,height_ratios=[1.2,1,1,1])
			gs.update(wspace=0.20,hspace=0.75)

	else:
		gs = gridspec.GridSpec(functN/2, 2)
		gs.update(wspace=0.20,hspace=0.5)
	if functN%2!=0:
		globals()[functL[0]](f,(fig.add_subplot(gs[0,:]),fitData,autolog,alpha,args[0][5]))
		r=1
		v=1
	for i in functL[v:]:
		globals()[i](f,(fig.add_subplot(gs[r,c]),fitData,autolog,alpha,args[0][5]))
		if c==1:
			c=0
			r+=1
		else:
			c+=1
	if epsMin==True:
		create_colormap()
		costPlot_min(f)
		fig.savefig(f.rName+".eps",bbox_inches="tight",format="eps")
	else:
		dateT=datetime.datetime.now().strftime('%b %d %I:%M %p %G')
		fig.savefig(f.rName+"_"+dateT+".eps",bbox_inches="tight",format="eps")

def create_colormap():
	newBounds=[1,4,8,12,16,32,128,256,1024,1800,6500]
	bounds=[-1,0,1,4,8,12,16,32,128,256,1024,2048,4000,sys.maxint-1,sys.maxint]
	cmap = plt.cm.jet
	norm = colors.BoundaryNorm(bounds, cmap.N)
	fig = plt.figure(figsize=(8,3))
	ax1 = fig.add_axes([0.05, 0.80, 0.9, 0.15])
	norm = colors.BoundaryNorm(bounds, cmap.N)
	cb1 = matplotlib.colorbar.ColorbarBase(ax1, cmap=cmap,norm=norm, boundaries=bounds, ticks=[1,16,256,4000],orientation="horizontal")
	plt.savefig("tmpmycolorbar.eps")
	plt.close()


def centroidP(x,y):
	centroid=(np.sum(x)/len(x),np.sum(y)/len(y))
	rx= np.max(x)-np.min(x)
	ry= np.max(y)-np.min(y)
	if rx>0 and ry>0:
		return 1.0*(centroid[0]-np.min(x))/(rx)<0.01 and 1.0*(centroid[1]-np.min(y))/(ry)<0.01
	return False

def costPlot(funct,args):
	pl=args[0]
	fitData=args[1]
	dat=args[4]
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,2])
	f=np.append(f,funct.rmsSet[:,5])
	if centroidP(x,y):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='y', style='sci', scilimits=(-2,5))
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.tick_params(axis='y', labelsize=7)
	pl.set_xlabel('read memory size',fontsize=8)
	pl.set_ylabel("cost",fontsize=8)
	pl.grid(True)
	pl.tick_params(axis='x', labelsize=7)
	sc=pl.scatter(x,y,s=6,c=f, marker = 'o',cmap=cmap,norm=norm,lw=0.0)
	fd=fitData[4]
	j=0
	curve=[]
	text=[]
	if funct.toFit:
		for i in fd:
			 tx=gen_samples(dat[j][4],dat[j][5])
			 cur=pl.plot(tx,i[0]+i[1]*np.power(tx,i[2]),c=np.random.rand(3,1),linewidth=2.5,label=str(round(i[2],2)))
			 j+=1
		if fitData[2]>0:
			pl.set_title("Cost: $\mathit{c = "+str(round(fitData[2],2))+",R{^2}="+str(round(fitData[3],3))+"}$",fontsize=14)
			pl.legend(loc=4)
		else:
			pl.set_title("Cost: No Fit Possible",fontsize=15)
	else:
		pl.set_title("Cost",fontsize=14)
	pylab.close()

def gen_samples(minx,maxx):
	result=np.array([])
	step=1.0*(maxx-minx)/500
	i=minx
	while i<maxx:
		result=np.append(result,i)
		i+=step		
	if i<maxx:
		result=np.append(result,maxx)
	return result
	
def costPlot_min(funct):
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,2])
	f=np.append(f,funct.rmsSet[:,5])
	plt.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	sc=plt.scatter(x,y,s=4,c=f, marker = 'o',cmap=cmap,norm=norm,lw=0.0)
	plt.axis("off")
	plt.savefig(funct.rName+"_min.png",dpi=25,transparent=True)
	plt.close()

		
	
def mamPlot(funct,args):
	pl=args[0]
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
	yavg=np.append(yavg,t1/t2)
	f=np.append(f,funct.rmsSet[:,5])
	if centroidP(x,yavg):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(ymax)+(2*np.amax(ymax)/100)])
	pl.set_xlabel('read memory size',fontsize=8)
	pl.set_ylabel("cost",fontsize=8)
	pl.grid(True)
	pl.set_title("Min/Avg/Max Cost",fontsize=14)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x,ymax,s=7,c='r', marker = 'o',lw=0.0)
	sc1=pl.scatter(x,yavg,s=5.5,c='g', marker = 'o',lw=0.0)	
	sc2=pl.scatter(x,ymin,s=4,c='b', marker = 'o',lw=0.0)	
	pl.legend((sc2,sc1,sc),("Min","Avg","Max"),scatterpoints=1,ncol=3,bbox_to_anchor=[0.5, mamAdjust],loc="lower center",fontsize=8)
	pylab.close()
	
def RMSFreqPlot(funct,args):
	pl=args[0]
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(f,funct.rmsSet[:,5])
	if centroidP(x,y):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([-2, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.set_xlabel("read memory size",fontsize=8)
	pl.set_ylabel("frequency",fontsize=8)
	pl.set_title("RMS Frequency",fontsize=14)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	pl.grid(True)
	sc=pl.scatter(x,y,s=6, marker = 'o',lw=0.0)
	pylab.close()

	
def TotalCostPlot(funct,args):
	pl=args[0]
	x=np.array([])
	y=np.array([])
	f=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	if centroidP(x,y):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([0, np.amax(x)+(2*np.amax(x)/100), 0, np.amax(y)+(2*np.amax(y)/100)])
	pl.set_xlabel("read memory size",fontsize=8)
	pl.set_ylabel("cost",fontsize=8)
	pl.grid(True)
	pl.set_title("Total Cost",fontsize=14)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x,y,s=6,c=f, marker = 'o',cmap=cmap,norm=norm,lw=0.0)
	pylab.close()
	#cbar=pylab.colorbar(sc,cmap=cmap, norm=norm, boundaries=newBounds, ticks=[],aspect=20,shrink=0.0,pad=0.00)




def AmortizedCostPlot(funct,args):
	#TODO: multi sort RMS
	pl=args[0]
	budget=0
	alpha=args[3]
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
	i=0
	while i<len(x):
		if y[i]<budget:
			budget-=y[i]
			z=np.append(z,0)
		else:
			z=np.append(z,((alpha+1)*y[i])/f[i])
			budget+=alpha*y[i]
		i+=1
	z=np.delete(z,0)
	if centroidP(x,z):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(z)+(10*np.amax(z)/100)])
	pl.set_xlabel("read memory size",fontsize=8)
	pl.set_ylabel("mean cumulative cost",fontsize=8)
	pl.set_title("Amortized Cost: $\mathit{\\alpha="+str(alpha)+"}$",fontsize=12)
	pl.grid(True)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x,z,s=6,marker = 'o',lw=0.0)
	pylab.close()

def CostVariancePlot(funct,args):
	pl=args[0]
	x=np.array([])
	y=np.array([])
	f=np.array([])
	z=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	z=np.append(z,funct.rmsSet[:,4])
	v=np.array([])
	v=np.append(v,[0])
	i=0
	while i<len(x):
		v=np.append(v,(z[i]/f[i])-(y[i]/f[i])*(y[i]/f[i]))
		i+=1
	v=np.delete(v,0)
	if centroidP(x,v):
		pl.set_yscale('log')
		pl.set_xscale('log') 
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([0, np.amax(x)+(10*np.amax(x)/100), 0, np.amax(v)+(10*np.amax(v)/100)])
	pl.set_xlabel("read memory size",fontsize=8)
	pl.set_ylabel("cost",fontsize=8)
	pl.set_title("Variance Cost",fontsize=14)
	pl.grid(True)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(x,v,c=f,s=6,marker = 'o',lw=0.0,cmap=cmap,norm=norm)
	pylab.close()		

def AlphaPlot(funct,args):
	#TODO: multi sort RMS
	pl=args[0]
	budget=0
	alpha=1.0
	x=np.array([])
	y=np.array([])
	f=np.array([])
	z=np.array([])
	p=np.array([])
	x=np.append(x,funct.rmsSet[:,0])
	y=np.append(y,funct.rmsSet[:,3])
	f=np.append(f,funct.rmsSet[:,5])
	z=np.append(z,[0])
	p=np.append(p,[0])
	num=0
	den=0
	count=0
	i=0
	budget=0
	pred=0
	while count!=len(x)-1:
		if count>pred:
			z=np.append(z,alpha)
			p=np.append(p,len(x)-count)
		alpha*=1.5 #a better granularity reduces performance
		pred=count
		count=0
		while i<len(x):
			if y[i]<budget:
				budget-=y[i]
				count+=1
			else:
				budget+=alpha*y[i]
			i+=1
		budget=0
		i=0
	if centroidP(z,p):
		pl.set_yscale('log')
		pl.set_xscale('log')
	else:
		pl.ticklabel_format(axis='both', style='sci', scilimits=(-2,5),pad=5,direction="bottom")
	pl.axis([0, np.amax(z)+(10*np.amax(z)/100), 0, np.amax(p)+(10*np.amax(p)/100)])
	pl.set_xlabel("alpha",fontsize=8)
	pl.set_ylabel("number of points",fontsize=8)
	pl.set_title("Alpha Plot",fontsize=14)
	pl.grid(True)
	pl.tick_params(axis='x', labelsize=7)
	pl.tick_params(axis='y', labelsize=7)
	sc=pl.scatter(z,p,s=4,marker = 'o',lw=0.0)
	pylab.close()

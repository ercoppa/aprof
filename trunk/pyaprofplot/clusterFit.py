import numpy as np
import math
import fitting as fit
import scipy.cluster.hierarchy as hcluster
import fastcluster as fc
import scipy.spatial.distance as ssd
import scipy.misc as deriv
np.seterr(all='call')
def errorhandler(errstr, errflag):
	#print errstr
	return [-1,-1,1],[0,0,0]
np.seterrcall(errorhandler)
def clusterFit(f):
	disabled=False
	try: 
		popt,pcov=fit.calc_fitVal(f.rmsSet[:,0],f.rmsSet[:,2])
		rs=RSquared2(f.rmsSet[:,0],f.rmsSet[:,2],popt)
		if rs>0.95 and popt[2]>0.85:
			final_result=np.array([0,0,0,0,0,0])
			final_result=np.vstack([final_result,(popt[0],popt[1],popt[2],rs,np.min(f.rmsSet[:,0]),np.max(f.rmsSet[:,0]))])
			final_result=np.delete(final_result,0,0)
			return final_result
		else:
			disabled=True
			return main_solver(f)
	except:
		if not disabled:
			return main_solver(f)



def RSquared2(x,y,p):
	mean_y=np.mean(y)# mean of y values
	TSS=0
	RSS=0
	index=0
	for i in y:
		TSS+=np.power(i-mean_y,2)
		RSS+=np.power(i-(p[0]+p[1]*np.power(x[index],p[2])),2)
		index+=1
	result=1-((RSS/TSS))
	return result
	
def square_split(x,y):
	minx=np.min(x)
	maxx=np.max(x)
	miny=np.min(y)
	maxy=np.max(y)
	slotx=1.0*(maxx-minx)/500
	sloty=1.0*(maxy-miny)/500
	result=np.array([[0,0]])
	for i in range(0,len(x)):
		nx=int(1.0*(x[i]-minx)/slotx)
		ny=int(1.0*(y[i]-miny)/sloty)
		if [nx,ny] not in result.tolist():
			result=np.vstack([result,(nx,ny)])
	result=np.delete(result,0,0)
	indx=result[:,0].argsort()
	result[:,0]=result[:,0][indx]
	result[:,1]=result[:,1][indx]
	
	return result[:,0],result[:,1],matrix_conv
	
	
def main_solver(f):
	x=np.array([],dtype="int32")
	y=np.array([],dtype="int32")
	x=np.append(x,f.rmsSet[:,0])
	y=np.append(y,f.rmsSet[:,2])
	originalSize=len(x)
	filt=False
	##################################
	# INVOCA LA FUNZIONE DI COMPRESSIONE DEI DATI
	#x,y=square_split(x,y)
	#filt=True
	################################
	newSize=len(x)
	k=0
	j=0
	dx=x
	dy=y
	draftx=np.array([])
	drafty=np.array([])
	result=np.array([[0,0,0,0,0]])
	data_covered=0
	oldxLen=0
	accx=np.array([],dtype="int32")
	k=0
	m=""
	indAcc=[]
	matrix=np.zeros(shape=(2,len(x)))
	matrix[0]=x
	matrix[1]=y
	th=0.02
	success=False
	dz=0.8
	nf=0
	pr=0
	while len(dx)>1 and ((len(dx)!=oldxLen) or (not success and dz>=th)):
		matrix=np.zeros(shape=(2,len(dx)))
		matrix[0]=dx
		matrix[1]=dy
		success=False
		if k>0 and len(dx)==oldxLen:
			dz=dz/2
		thold_fit=int(dz*len(dx))
		try:
			Y=ssd.pdist(np.transpose(matrix),metric="mahalanobis")
		except:
			Y=ssd.pdist(np.transpose(matrix),metric="euclidean")
		tree=hcluster.to_tree(fc.linkage(Y,method="single"))
		sta,d,av,check,tnf,tpr=solver(tree,dx,dy,tree.get_count(),thold_fit,0,j)
		nf+=tnf
		pr+=tpr
		oldxLen=len(dx)
		for i in d:
			dx=np.setdiff1d(dx,i[5])
			if 1.0*(np.max(i[0][:,0])-np.min(i[0][:,0]))/(np.max(x)-np.min(x))>0.01:
				success=True
				data_covered+=len(i[5])
				result=np.vstack([result,(i[0],i[7],i[4],i[8],np.min(i[0][:,0]))])
			else:
				draftx=np.concatenate((draftx,i[0][:,0]))
				drafty=np.concatenate((drafty,i[0][:,1]))
		k+=1
		dx=dx[dx.argsort()]		
		indx=np.searchsorted(x,dx)
		dx=x[indx]
		dy=y[indx]
	result=np.delete(result,0,0)
	if len(result)>0:
		result=result[result[:,4].argsort()]
	else:
		return []
	epsilon=2.0
	dx=np.concatenate((dx,draftx))
	dy=np.concatenate((dy,drafty))
	indx=dx.argsort()
	dx=dx[indx]
	dy=dy[indx]
	for j in result:
		to_merge=np.array([[0,0]])
		saved=np.array([],dtype="int32")
		for i in  range(0,len(dx)):
			epsilon=1.0* (j[1][0]+j[1][1]*np.power(dx[i],j[1][2]))/40
			if j[1][0]+j[1][1]*np.power(dx[i],j[1][2])<=dy[i]+epsilon and j[1][0]+j[1][1]*np.power(dx[i],j[1][2])>=dy[i]-epsilon:
				to_merge=np.vstack([to_merge,(dx[i],dy[i])])
				data_covered+=1
			else:
				saved=np.append(saved,i)
		to_merge=np.delete(to_merge,0,0)
		j[0]=merge_fit(j[0],to_merge)
		j[4]=np.max(j[0][:,1])
		dx=dx[saved]
		dy=dy[saved]
	result=result[result[:,4].argsort()]
	result=result[::-1]
	merged=False
	modified=True
	init=len(result)
	r=np.array([[0,0,0,0,0]])
	merged_set=[]
	while modified:
		r=np.array([[0,0,0,0,0]])
		modified=False
		merged_set=[]
		for i in range(0,len(result)):
			merged=False
			toapp=result[i]
			if i not in merged_set:
				for j in range(i+1,len(result)):
					tr=cheb_test2(toapp[0:4],result[j][0:4])
					if len(tr)>0:
						modified=True
						merged_set.append(j)
						toapp=(tr[0],tr[1],tr[2],tr[3],np.max(tr[0][:,1]))
				r=np.vstack([r,toapp])
		r=np.delete(r,0,0)
		r=r[r[:,4].argsort()]
		r=r[::-1]
		result=r
	final_result=np.array([0,0,0,0,0,0])
	for i in result:
		final_result=np.vstack([final_result,(i[1][0],i[1][1],i[1][2],i[2],np.min(i[0]),np.max(i[0]))])
	final_result=np.delete(final_result,0,0)
	if len(final_result)>0:
		final_result=final_result[final_result[:,2].argsort()]
		final_result=final_result[::-1]
	return final_result

def solver(cluster,x,y,n,th,liv,it):
	if cluster.get_count()==1:
		currL=np.array([[x[cluster.pre_order()[0]],y[cluster.pre_order()[0]]]],dtype="int32")
		return currL,[],0,True,0,0
	derivCheck=True
	left,ldata,avl,lcheck,lf,lpr=solver(cluster.get_left(),x,y,n,th,liv+1,it)
	right,rdata,avr,rcheck,rf,rpr=solver(cluster.get_right(),x,y,n,th,liv+1,it)
	currL=np.array([[0,0]],dtype="int32")
	tot_data=ldata+rdata
	av=avl+avr
	nfit=lf+rf
	pruned=lpr+rpr
	if lcheck==False or rcheck==False:
		return [],tot_data,av,False,nfit,pruned
	currL=merge_fit(left,right)
	if len(currL)==0:
		return [],[],av,True,nfit,pruned
	if len(currL[:,0])>=th: #
		nfit+=1
		try: 
			popt,pcov=fit.calc_fitVal(currL[:,0],currL[:,1])
			if derivative_check(currL[:,0],popt,left,right):
				rs=RSquared2(currL[:,0],currL[:,1],popt)
				if rs>=0.95 and popt[2]>0.9:
					tot_data=[(currL,round(popt[0],2),round(popt[1],2),round(popt[2],2),round(rs,3),currL[:,0],currL[:,1],popt,0)]
			else:
				if liv>0:
					pruned+=liv-1
				derivCheck=False
		except:
			pass
	return	currL,tot_data,av,derivCheck,nfit,pruned
	
	
def derivative_check(x,popt,left,right):
	if len(left)==0 or len(right)==0:
		return True
	if len(left)==1 or len(right)==1:
		return True
	maxlx=np.max(left[:,0])
	minlx=np.min(left[:,0])
	maxly=np.max(left[:,1])
	minly=np.min(left[:,1])
	maxrx=np.max(right[:,0])
	minrx=np.min(right[:,0])
	maxry=np.max(right[:,1])
	minry=np.min(right[:,1])
	t1=-1
	t2=-1
	pend1=[]
	pend2=[]
	pend1.append(deriv.derivative(fu,minlx,n=1,args=popt))
	pend1.append(deriv.derivative(fu,maxlx,n=1,args=popt))
	pend1.append(deriv.derivative(fu,minrx,n=1,args=popt))
	pend1.append(deriv.derivative(fu,maxrx,n=1,args=popt))
	sign=pend1[0]
	for i in pend1:
		if i<0:
			return False
	pend1=[]
	pend1.append(deriv.derivative(fu,minlx,n=2,args=popt))
	pend1.append(deriv.derivative(fu,maxlx,n=2,args=popt))
	pend1.append(deriv.derivative(fu,minrx,n=2,args=popt))
	pend1.append(deriv.derivative(fu,maxrx,n=2,args=popt))
	sign=pend1[0]
	for i in pend1:
		if (sign>0 and i<0) or (sign<0 and i>0):
			return False
	return True

def fu(x,a,b,c):
	return a+b*(x**c)
	


def merge(leftG,rightG,containment,inters):
	left=leftG[0]
	right=rightG[0]
	temp=merge_fit(left,right)
	recovery=False
	try:
		popt,pcov=fit.calc_fitVal(temp[:,0],temp[:,1])
		if derivative_check(temp[:,0],popt,left,right):
			rs=RSquared2(temp[:,0],temp[:,1],popt)
			if rs>=0.95 and popt[2]>0.9:
				return (temp,popt,rs,0)
			else:
				recovery=True
		else:
			recovery=True
	except:
		recovery=True
		pass
	return []

def cheb_test2(leftG,rightG):
	left=leftG[0]
	right=rightG[0]
	if len(left)==0 or len(right)==0:
		return True
	llen=len(left[:,0])-1
	rlen=len(right[:,0])-1
	maxlx=left[:,0][llen]
	minlx=left[:,0][0]
	maxly=left[:,1][llen]
	minly=left[:,1][0]
	maxrx=right[:,0][rlen]
	minrx=right[:,0][0]
	maxry=right[:,1][rlen]
	minry=right[:,1][0]
	passed=False
	inters=False
	containment=False

	if (maxlx<minrx)and not(ct(right,maxlx,maxly)):
		passed=True
	elif (maxrx<minlx) and not (ct(left,maxrx,maxry)):
		passed=True
	elif maxlx>=minrx and maxlx<=maxrx:
		if minlx>=minrx:
			rx=left[:,0]
			ry=left[:,1]
			check=True
			for i in range(0,len(rx)):
				if ct(right,rx[i],ry[i]):
					check=False
					break

			if check==True:
				passed=True
				containment=True
				container=rightG
				contained=leftG
		else:
			if not(ct(left,minrx,minry)) and not(ct(right,maxlx,maxly)):
				inters=True
				passed=True
	
	elif maxrx>=minlx and maxrx<=maxlx:
		if minrx>=minlx:
			rx=right[:,0]
			ry=right[:,1]
			check=True
			for i in range(0,len(rx)):
				if ct(left,rx[i],ry[i]):
					check=False
					break

			if check==True:
				passed=True
				containment=True
				container=leftG
				contained=rightG

		else:
			if not(ct(left,maxrx,maxry)) and not(ct(right,minlx,minly)) :
				inters=True
				passed=True
	if passed:
		if containment:
			return merge(contained,container,containment,inters)
		return merge(leftG,rightG,containment,inters)
	return []

def merge_fit(left,right):
	x=left[:,0]
	y=left[:,1]
	result=np.array([[0,0]],dtype="int32")
	for i in range(0,len(x)):
		j=(x[i],y[i])
		result=np.vstack([result,j])
	x=right[:,0]
	y=right[:,1]	
	for i in range(0,len(x)):
		j=(x[i],y[i])
		result=np.vstack([result,j])
	result=np.delete(result,0,0)
	result=result[result[:,0].argsort()]
	return result
	
def ct(dataset,px,py):
	x=dataset[:,0]
	y=dataset[:,1]
	mean_y=np.mean(y)
	std_y=np.std(y)
	mean_x=np.mean(x)
	std_x=np.std(x)
	if (std_y>0.0 and abs((1.0*(py-mean_y))/std_y)>3) or  (std_x>0.0 and abs((1.0*(px-mean_x))/std_x)>3):
		return True
	return False

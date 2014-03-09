import AFunct as func
import infoCon as infoC
import functionSet as fSet
import argparse
import tex_generator as texG
import os
import shutil
from multiprocessing import Process
import multiprocessing
import numpy as np
import sys
import cfg_manager as cfgm
def main():
	parser = argparse.ArgumentParser(description="A tool that generates a pdf to see the Aprof response for main functions inside a program")
	parser.add_argument("fileIN", type=str, help="the aprof file")
	parser.add_argument("-loadcfg", "--loadConfig",  type=str,help="Load a configuration in the cfg folder",metavar='')
	parser.add_argument("-savecfg", "--saveConfig",  type=str,help="Save the current configuration in the cfg folder",metavar='')
	parser.add_argument("-eps", "--EPS",  nargs ='+',help="Save only the .eps of routines; First set the folder location, then write the name of all the routines you want to save")
	parser.add_argument("-r", "--RMS", type=int,help="#RMS treeshold (default: 5)",metavar='')
	parser.add_argument("-cp", "--CPERC", type=float,help="Cumulative Cost Percentage treeshold per routine (default: 1)",metavar='')
	parser.add_argument("-f", "--Freq",help="creates only frequency plot (default: off)",action="store_true")
	parser.add_argument("-c", "--Cost",help="creates only cost plot (default: off)",action="store_true")
	parser.add_argument("-tc", "--TotalCost",help="creates only total cost plot (default: off)",action="store_true")
	parser.add_argument("-ap", "--AlphaPlot",help="creates only Alfa plot (default: off)",action="store_true")
	parser.add_argument("-amc", "--AmortizedCost",help="creates only amortized cost plot (default: off)",metavar="")
	parser.add_argument("-mam", "--MinAvgMaxCost",help="creates only min/avg/max cost plot (default: off)",action="store_true")
	parser.add_argument("-cv", "--CostVariance",help="creates only cost variance plot (default: off)",action="store_true")
	parser.add_argument("-al", "--Autolog",help="plot curves in log-log coordinates if centroid is under 0.1 percent of total range",action="store_true")
	parser.add_argument("-nf", "--noFit",help="Disable the application of fit algorithm on the max cost cirve (Default: False)",action="store_true")
	parser.add_argument("-s", "--Sort",help="Sort routines with respect to one of the available criteria (Default:fit)",choices=["fit","name","rms","cumul%","lib","cumul","calls","calls%"])
	parser.add_argument("-tf", "--toFit",  nargs ='+',help="apply the fitting algorithm on the selected routines")
	args = parser.parse_args()
	functToPlot={"rms frequency":True,"cost":True,"total cost":True,"amortized cost":True,"minavgmax":True,"cost variance":True,"alpha plot":True}
	filters={"rms":5,"cumul cost percentage":1.0}
	others={"autolog":False,"alpha":1.0,"noFit":False,"sort":"fit"}
	treesholdRMS=5
	treesholdCPERC=1
	epsB=False
	toPlot=[]
	r_toFit=[]
	filterFit=False
	fileIN = args.fileIN
	if len(sys.argv)>2:
		if args.loadConfig!=None:
			functToPlot,filters,others=cfgm.loadConfig(args.loadConfig)
		elif args.Freq or args.Cost or args.TotalCost or args.MinAvgMaxCost or args.AmortizedCost or args.CostVariance or args.AlphaPlot:
			if args.Freq==False:
				functToPlot["rms frequency"]=False
			if args.Cost==False:
				functToPlot["cost"]=False		
			if  args.TotalCost==False:
				functToPlot["total cost"]=False
			if args.AmortizedCost>=0:
				functToPlot["amortized cost"]=True
				others["alpha"]=float(args.AmortizedCost)
			else:
				functToPlot["amortized cost"]=False
			if args.CostVariance==False:
				functToPlot["cost variance"]=False
			if args.MinAvgMaxCost==False:
				functToPlot["minavgmax"]=False
			if args.AlphaPlot==False:
				functToPlot["alpha plot"]=False
		if args.RMS > 0:
			filters["rms"]=int(args.RMS)
		if args.CPERC >= 0:
			filters["cumul cost percentage"]=args.CPERC
		if args.EPS and len(args.EPS)>=2:
			epsB=True
		if args.Autolog:
			others["autolog"]=True
		if args.noFit:
			others["noFit"]=True
		if args.Sort!=None:
			others["sort"]=args.Sort
		if args.saveConfig!=None:
			cfgm.create_config(args.saveConfig,functToPlot,filters,others)
		if args.toFit and len(args.toFit)>=1:
			filterFit=True
			for i in args.toFit:
				r_toFit.append(i)
			
	CORES_NUMBER=multiprocessing.cpu_count()
	with open(fileIN) as f:
		data = f.read()

	data = data.split('\n')
	tuples_list=[]
	for i in data:
		tuples_list.append(i.split())
	temp=[]
	totCalls=0
	totCumul=0
	title=""
	for i in tuples_list:
		if i:
			if i[0]=="v":
				vn=i[1]
			elif i[0]=="m":
				m=i[1]
			elif i[0]=="e":
				ex=i[1]
			elif i[0]=="k":
				tc=int(i[1])
			elif i[0]=="f":
				title=i[1][2:]
			elif i[0]=="r":
				libName=(i[2].split("/"))[-1]
				if others["noFit"]==True:
					f=func.AFunct(str(i[1][1:len(i[1])-1]),str(i[2][1:len(i[2])-1]),int(i[3]),libName[0:len(libName)-1],False)
				else:
					if len(r_toFit)==0 and not filterFit:
						f=func.AFunct(str(i[1][1:len(i[1])-1]),str(i[2][1:len(i[2])-1]),int(i[3]),libName[0:len(libName)-1],True)
					elif str(i[1][1:len(i[1])-1]) in r_toFit:
						f=func.AFunct(str(i[1][1:len(i[1])-1]),str(i[2][1:len(i[2])-1]),int(i[3]),libName[0:len(libName)-1],True)
					else:
						f=func.AFunct(str(i[1][1:len(i[1])-1]),str(i[2][1:len(i[2])-1]),int(i[3]),libName[0:len(libName)-1],False)		 
				temp.append(f)
			elif i[0]=="p":
				totCalls+=int(i[7])
				f.addRms(i)
	aggregator=infoC.infoCon(vn,tc,ex,m,totCalls)
	for i in temp:
		i.reorder()
		aggregator.addFunct(i)
	count=0
	retval = os.getcwd()
	if epsB==True:
		if not os.path.exists(str(args.EPS[0])):
			os.makedirs(str(args.EPS[0]))
		os.chdir(str(args.EPS[0]))
	else:	
		if not os.path.exists("tmp"):
			os.mkdir("tmp")
		else:
			shutil.rmtree("tmp")
			os.mkdir("tmp")
		os.chdir("tmp")
	if functToPlot.get("cost"):
		toPlot.append("costPlot")
	if functToPlot.get("total cost"):
		toPlot.append("TotalCostPlot")
	if functToPlot.get("rms frequency"):
		toPlot.append("RMSFreqPlot")
	if functToPlot.get("amortized cost"):
		toPlot.append("AmortizedCostPlot")
	if functToPlot.get("cost variance"):
		toPlot.append("CostVariancePlot")
	if functToPlot.get("alpha plot"):
		toPlot.append("AlphaPlot")
	if functToPlot.get("minavgmax"):
		toPlot.append("mamPlot")
	proc_args=[] 
	currProc=0
	pool=multiprocessing.Pool(CORES_NUMBER)
	if epsB==True:
		toSave=args.EPS[1:]
		for i in aggregator.functContainer:
			if i.rName in toSave:
				proc_args.append(i)
	else:
		for i in aggregator.functContainer:
			if i.Nrms>=int(filters["rms"]) and i.cumulativeCostPerc>=float(filters["cumul cost percentage"]):
					proc_args.append(i)
	result=[]
	async=pool.map_async(fSet.fitParameters,proc_args,callback=result.append)
	async.wait()
	sortV=cfgm.sort_criteria(others["sort"])
	sortedRoutines,data=fitData_manager(result[0],toPlot,epsB,others["autolog"],float(others["alpha"]),sortV)
	pool=multiprocessing.Pool(CORES_NUMBER)
	pool.map(fSet.create_grid,sortedRoutines)
	os.chdir(retval)
	if epsB==False:
		texG.tex_gen(title,data,len(toPlot),(filters,others))

def fitData_manager(ar,toPlot,epsB,autolog,alpha,sortV):
	dataM=np.array([[0,0,0,0,0,0,0,0,0,0,0,0,0]])
	for i in ar:
		dataM=np.vstack([dataM,i])
	dataM=np.delete(dataM,0,0)
	dataM=dataM[dataM[:,sortV].argsort()]
	if sortV!=6 and sortV!=7:
		dataM=dataM[::-1] #reversed
	result=[]
	for i in dataM:
		result.append((i.tolist(),toPlot,not epsB,autolog,alpha))
	return result,dataM[:,0]	

if __name__ == "__main__":
    main()

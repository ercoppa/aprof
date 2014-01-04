import AFunct as func
import infoCon as infoC
import functionSet as fSet
import argparse
import tex_generator as texG
import os
import shutil

parser = argparse.ArgumentParser(description="A tool that generates a pdf to see the Aprof response for main functions inside a program")
parser.add_argument("fileIN", type=str, help="the aprof file")
parser.add_argument("-r", "--RMS", type=int,help="#RMS treeshold (default: 5)",metavar='')
parser.add_argument("-a", "--All",help="creates all the possible plots except the curve bounding one (default: on)",action="store_true")
parser.add_argument("-f", "--Freq",help="creates only frequency plot (default: off)",action="store_true")
parser.add_argument("-c", "--Cost",help="creates only cost plot (default: off)",action="store_true")
parser.add_argument("-tc", "--TotalCost",help="creates only total cost plot (default: off)",action="store_true")
parser.add_argument("-mcc", "--MeanCumulCost",help="creates only mean cumulative cost plot (default: off)",action="store_true")
parser.add_argument("-mam", "--MinAvgMaxCost",help="creates only min/avg/max cost plot (default: off)",action="store_true")
parser.add_argument("-cb", "--CurveBounding",type=str,help="creates only curve bounding plot (default: off)",choices=['n','nlogn','logn'])
args = parser.parse_args()
aP=False
fP=False
cP=False
tcP=False
cbP=False
mccP=False
mamP=False
functB=""
fileIN = args.fileIN
if args.All or (not(args.Freq or args.Cost or args.TotalCost or args.CurveBounding or args.MinAvgMaxCost)):
	fP=True
	tcP=True
	cP=True
	mccP=True
	mamP=True
	if args.CurveBounding:
		cbP=True
		functB=args.CurveBounding	
else:
	if args.Freq:
		fP=True
	if args.Cost:
		cP=True
	if args.TotalCost:
		tcP=True
	if args.CurveBounding:
		cbP=True
		functB=args.CurveBounding
	if args.MeanCumulCost:
		mccP=True
	if args.MinAvgMaxCost:
		mamP=True
if args.RMS > 0:
    treeshold=args.RMS
else:
	treeshold=5

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
			f= func.AFunct(str(i[1][1:len(i[1])-1]),str(i[2][1:len(i[2])-1]),int(i[3]),libName[0:len(libName)-1])
			temp.append(f)
		elif i[0]=="p":
			totCalls+=int(i[7])
			f.addRms(i)
aggregator=infoC.infoCon(vn,tc,ex,m,totCalls)
for i in temp:
	i.reorder()
	aggregator.addFunct(i)

data=[]
count=0
if not os.path.exists("tmp"):
	os.mkdir("tmp")
else:
	shutil.rmtree("tmp")
	os.mkdir("tmp")
toPlot=[]
if cP==True:
	toPlot.append("costPlot")
if tcP==True:
	toPlot.append("TotalCostPlot")
if fP==True:
	toPlot.append("RMSFreqPlot")
if cbP==True:
	toPlot.append("curveBounding")
if mccP==True:
	toPlot.append("MeanCumulativePlot")
if mamP==True:
	toPlot.append("mamPlot")

for i in aggregator.functContainer:
	if i.Nrms>=treeshold:

		data.append(i)
		fSet.create_grid(i,toPlot)

texG.tex_gen(title,data)


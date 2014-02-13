import ConfigParser as confP
import os


def create_config(name,funct,fil,ot):
	if not os.path.exists("cfg"):
			os.mkdir("cfg")
	config_to_save = confP.RawConfigParser()
	config_to_save.add_section('Functions')
	config_to_save.add_section('Filters')
	config_to_save.add_section('Others')
	config_to_save.set('Functions',"Cost","True")
	for i in funct:
		config_to_save.set('Functions',i,funct.get(i))
	for j in fil:
		config_to_save.set('Filters',j,fil.get(j))
	for i in ot:
		config_to_save.set('Others',i,ot.get(i))		
	with open('cfg/'+name+'.cfg', 'wb') as configfile:
		config_to_save.write(configfile)
		

def loadConfig(name):
	config = confP.RawConfigParser()
	config.read('cfg/'+name+'.cfg')
	funct={"rms frequency":config.getboolean("Functions","rms frequency"),"cost":config.getboolean("Functions","cost")
	,"total cost":config.getboolean("Functions","total cost"),"amortized cost":config.getboolean("Functions","amortized cost")
	,"minavgmax":config.getboolean("Functions","minavgmax"),"cost variance":config.getboolean("Functions","cost variance"),"alpha plot":config.getboolean("Functions","alpha plot")}
	fil={"rms":config.getint("Filters","rms"),"cumul cost percentage":config.getfloat("Filters","cumul cost percentage")}
	ot={"autolog":config.getboolean("Others","autolog"),"alpha":config.getfloat("Others","alpha"),"noFit":config.getboolean("Others","noFit"),"sort":config.get("Others","sort")}
	return funct,fil,ot

def sort_criteria(num):
	if num==None or num=="fit":
		return 3
	if num=="name":
		return 6
	if num=="lib":
		return 7
	if num=="rms":
		return 8
	if num=="cumul":
		return 9
	if num=="cumul%":
		return 10
	if num=="calls":
		return 11
	if num=="calls%":
		return 12

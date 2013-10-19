import datetime
import subprocess
import os
from glob import glob
import re
import shutil

def tex_gen(t,info):
	title=t+"'s Aprof Profile"
	author="by Aprof Report Generator"
	retval = os.getcwd()
	os.chdir("tmp")

	dateT=datetime.datetime.now().strftime('%b %d %I:%M %p %G')
	tex_file = open(t+"_report.tex","w")
	tex_file.write("\documentclass [letterpaper,12pt,openany,oneside]{book}\n")
	tex_file.write("\usepackage{graphicx}\n")
	tex_file.write("\usepackage{epstopdf}\n")
	tex_file.write("\usepackage{ragged2e}\n")
	tex_file.write("\usepackage{caption}\n")
	tex_file.write("\usepackage{subcaption}\n")
	tex_file.write("\usepackage{titlesec}\n")
	tex_file.write("\\newcommand{\sectionbreak}{\clearpage}\n")
	tex_file.write("\\title{"+title.replace('_','\\textunderscore ')+"}\n")
	tex_file.write("\\author{"+author+"}\n")
	tex_file.write("\date{"+dateT+"}\n")
	tex_file.write("\pagestyle{plain}\n")
	tex_file.write("\\begin{document}\n")
	tex_file.write("\maketitle\n")
	tex_file.write("\\tableofcontents\n")
	tex_file.write("\chapter{How to read it}\n")
	tex_file.write("\chapter{Routines Summary}\n")
	tex_file.write("\chapter{Routines Charts}\n")
	for i in info:
		name=i.replace('_','\\textunderscore ')
		tex_file.write("\section{"+name+"}\n")
		tex_file.write("\includegraphics[width=\\textwidth]{"+i+".eps"+"}\n")

	tex_file.write("\end{document}\n")
	
	tex_file.close()
	return_value = subprocess.call(['pdflatex',t+"_report.tex"], shell=False)
	return_value = subprocess.call(['pdflatex',t+"_report.tex"], shell=False)
	shutil.copy2(os.getcwd()+"/"+t+"_report.pdf", retval)
	os.chdir(retval)
	shutil.rmtree("tmp")

	

	

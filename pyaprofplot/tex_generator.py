import datetime
import subprocess
import os
from glob import glob
import re
import shutil
import locale

def tex_gen(t,info):
	title=t+"'s Aprof Profile"
	author="by Aprof Report Generator"
	retval = os.getcwd()
	os.chdir("tmp")
	locale.setlocale(locale.LC_ALL, '')
	dateT=datetime.datetime.now().strftime('%b %d %I:%M %p %G')
	tex_file = open(t+"_report.tex","w")
	tex_file.write("\documentclass [letterpaper,12pt,openany,oneside]{book}\n")
	tex_file.write("\usepackage{array}\n")
	tex_file.write("\usepackage{longtable}\n")
	tex_file.write("\usepackage{graphicx}\n")
	tex_file.write("\usepackage{epstopdf}\n")
	tex_file.write("\usepackage{ragged2e}\n")
	tex_file.write("\usepackage{caption}\n")
	tex_file.write("\usepackage{subcaption}\n")
	tex_file.write("\usepackage{titlesec}\n")
	tex_file.write("\usepackage{seqsplit}\n")
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
	tex_file.write("\scriptsize")
	tex_file.write("\\begin{center}")
	tex_file.write("\\begin{longtable}{|>{\hspace{0pt}\centering\\arraybackslash}m{0.09\\textwidth}|>{\centering\\arraybackslash}m{0.11\\textwidth}|>{\centering\\arraybackslash}m{0.07\\textwidth}|>{\centering\\arraybackslash}m{0.09\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.09\\textwidth}|}\n")
	tex_file.write("\hline\n")
	tex_file.write("\\textbf{Routine} & \\textbf{Cost Plot \scriptsize(Cumul.) \\normalsize} & \\textbf{Lib} & \\textbf{\# RMS} & \\textbf{Cost \scriptsize(Cumul.)\\normalsize} & \\textbf{Cost \% \scriptsize(Cumul.)\\normalsize} & \\textbf{Calls} & \\textbf{Calls \%} \\\\")
	tex_file.write("\hline\n")
	tex_file.write("\endfirsthead\n")
	tex_file.write("\multicolumn{8}{c}\n")
	tex_file.write("{\\tablename\ \\thetable\ -- \\textit{Continued from previous page}} \\\\")
	tex_file.write("\hline\n")
	tex_file.write("\\textbf{Routine} & \\textbf{Cost Plot \scriptsize(Cumul.) \\normalsize} & \\textbf{Lib} & \\textbf{\# RMS} & \\textbf{Cost \scriptsize(Cumul.)\\normalsize} & \\textbf{Cost \% \scriptsize(Cumul.)\\normalsize} & \\textbf{Calls} & \\textbf{Calls \%} \\\\")
	tex_file.write("\endhead\n")
	tex_file.write("\hline\n")
	tex_file.write("\multicolumn{8}{r}{\\textit{Continued on next page}} \\\\")
	tex_file.write("\endfoot\n")
	tex_file.write("\hline\n")
	tex_file.write("\endlastfoot\n")

	for i in info:
		tex_file.write("\seqsplit{"+i.rName.replace('_','\\textunderscore \\allowbreak ')+"} & "+"\includegraphics[width=0.14\\textwidth]{{"+i.rName+"_min}.eps"+"}\n"+" & "+i.lib.replace('_','\\textunderscore \\allowbreak ')+" & "+"{:,}".format(i.Nrms)+" & "+"{:,}".format(i.cumulativeCost)+" & "+str(i.cumulativeCostPerc)+" & "+"{:,}".format(i.totCalls)+" & "+str(i.totCallsPerc)+" \\\\")
		tex_file.write("\hline\n")
	tex_file.write("\end{longtable}\n")
	tex_file.write("\end{center}\n")
	tex_file.write("\\normalsize")
	tex_file.write("\chapter{Routines Charts}\n")
	tex_file.write("\\newpage\n")
	for i in info:
		tex_file.write("\section{"+i.rName.replace('_','\\textunderscore ')+"}\n")
		tex_file.write("\\begin{center}")
		tex_file.write("\\resizebox{1.0\\textwidth}{!}{\n")
		tex_file.write("\\begin{tabular}{|c|c|c|c|c|c|}\n")
		tex_file.write("\hline\n")
		tex_file.write("\\textbf{Lib}  & \\textbf{\# RMS} & \\textbf{Cost (Cumul.)} & \\textbf{Cost \% (Cumul.)} & \\textbf{Calls} & \\textbf{Calls \%} \\\\")
		tex_file.write("\hline\n")
		tex_file.write(i.lib.replace('_','\\textunderscore ')+" & "+"{:,}".format(i.Nrms)+" & "+"{:,}".format(i.cumulativeCost)+" & "+str(i.cumulativeCostPerc)+" & "+"{:,}".format(i.totCalls)+" & "+str(i.totCallsPerc)+" \\\\")
		tex_file.write("\hline\n")
		tex_file.write("\end{tabular}\n")
		tex_file.write("}\n")
		tex_file.write("\includegraphics[width=\\textwidth]{{"+i.rName+"}.eps"+"}\n")
		tex_file.write("\end{center}\n")


	tex_file.write("\end{document}\n")
	
	tex_file.close()
	return_value = subprocess.call(['pdflatex',t+"_report.tex"], shell=False)
	return_value = subprocess.call(['pdflatex',t+"_report.tex"], shell=False)
	shutil.copy2(os.getcwd()+"/"+t+"_report.pdf", retval)
	os.chdir(retval)
	shutil.rmtree("tmp")

	

	

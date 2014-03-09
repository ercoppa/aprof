import datetime
import subprocess
import os
from glob import glob
import re
import shutil
import locale

def tex_gen(t,info,tp,readInfo):
	title=t+"'s Aprof Profile"
	author="by Aprof Report Generator"
	retval = os.getcwd()
	os.chdir("tmp")
	dateT=datetime.datetime.now().strftime('%b %d %I:%M %p %G')
	tex_file = open(t.replace("/","_s_")+"_report.tex","w")
	tex_file.write("\documentclass [letterpaper,10pt,openany,oneside]{book}\n")
	tex_file.write("\usepackage[margin=1.5cm]{geometry}\n")
	tex_file.write("\usepackage{mathptmx}\n")
	tex_file.write("\usepackage{multicol}\n")
	tex_file.write("\usepackage{array}\n")
	tex_file.write("\usepackage{amsmath}\n")
	tex_file.write("\usepackage{longtable}\n")
	tex_file.write("\usepackage{graphicx}\n")
	tex_file.write("\usepackage{epstopdf}\n")
	tex_file.write("\usepackage{seqsplit}\n")
	tex_file.write("\usepackage{subcaption}\n")
	tex_file.write("\usepackage{hyperref}\n")
	tex_file.write("\\newcommand{\sectionbreak}{\clearpage}\n")
	tex_file.write("\\title{"+title.replace('_','\\textunderscore ')+"}\n")
	tex_file.write("\\author{"+author+"}\n")
	tex_file.write("\date{"+dateT+"}\n")
	tex_file.write("\pagestyle{plain}\n")
	tex_file.write("\hfuzz 5.00pt\n")
	tex_file.write("\\begin{document}\n")
	tex_file.write("\maketitle\n")
	tex_file.write("\\tableofcontents\n")
	tex_file.write("\chapter{How to read it}\n")
	tex_file.write("\\begin{center}")
	tex_file.write("Except for {\em Frequency}, {\em Amortized}, {\em Min/Max/Avg} and {\em Alpha} plots, the following colorbar represents the RMS frequencies:\n")
	tex_file.write("\\newline")
	tex_file.write("\\newline")
	tex_file.write("\\newline")
	tex_file.write("\includegraphics[width=0.5\\linewidth]{tmpmycolorbar.eps"+"}\n")
	tex_file.write("\end{center}")
	tex_file.write("\chapter{Routines Summary}\n")
	tex_file.write("\scriptsize")
	tex_file.write("\\begin{center}")
	tex_file.write("\\begin{longtable}{|>{\hspace{0pt}\centering\\arraybackslash}m{0.09\\textwidth}|>{\hspace{0pt}\centering\\arraybackslash}m{0.11\\textwidth}|>{\hspace{0pt}\centering\\arraybackslash}m{0.07\\textwidth}|>{\centering\\arraybackslash}m{0.09\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.1\\textwidth}|>{\centering\\arraybackslash}m{0.09\\textwidth}|}\n")
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
		tex_file.write("\seqsplit{"+i.rName.replace('_','\\textunderscore \\allowbreak ')+"} & "+"\includegraphics[width=0.11\\textwidth]{{"+i.rName+"_min}.png"+"}\n"+" & "+i.lib.replace('_','\\textunderscore \\allowbreak ').replace(".","\. \\allowbreak ")+" & "+"{:,}".format(i.Nrms)+" & "+"{:,}".format(i.cumulativeCost)+" & "+str(i.cumulativeCostPerc)+" & "+"{:,}".format(i.totCalls)+" & "+str(i.totCallsPerc)+" \\\\")
		tex_file.write("\hline\n")
	tex_file.write("\end{longtable}\n")
	tex_file.write("\end{center}\n")
	tex_file.write("\\normalsize")
	tex_file.write("\chapter{Routines Charts}\n")
	k=0
	j=0
	first_page=True
	if tp>3:
		rpp=2
	else:
		rpp=4
	for i in info:
		if k==0:
			tex_file.write("\\begin{figure}[ht]\n")	
		k+=1		
		tex_file.write("\\begin{subfigure}[b]{0.5\linewidth}\n")
		tex_file.write("\section{"+i.rName.replace('_','\\textunderscore ')+"}\n")
		tex_file.write("\centering")
		tex_file.write("\\resizebox{0.98\\textwidth}{!}{\n")
		tex_file.write("\\begin{tabular}{|c|c|c|c|c|c|}\n")
		tex_file.write("\hline\n")
		tex_file.write("\\textbf{Lib}  & \\textbf{\# RMS} & \\textbf{Cost (Cumul.)} & \\textbf{Cost \% (Cumul.)} & \\textbf{Calls} & \\textbf{Calls \%} \\\\")
		tex_file.write("\hline\n")
		tex_file.write(i.lib.replace('_','\\textunderscore ')+" & "+"{:,}".format(i.Nrms)+" & "+"{:,}".format(i.cumulativeCost)+" & "+str(i.cumulativeCostPerc)+" & "+"{:,}".format(i.totCalls)+" & "+str(i.totCallsPerc)+" \\\\")
		tex_file.write("\hline\n")
		tex_file.write("\end{tabular}\n")
		tex_file.write("\\newline\n")
		tex_file.write("}\n")
		tex_file.write("\includegraphics[width=0.98\\linewidth]{{"+i.rName+"}.eps"+"}\n") 
		tex_file.write("\\vspace{4ex}\n")
		tex_file.write("\end{subfigure}\n")			
		if k==rpp:
			k=0
			tex_file.write("\end{figure}\n")
			tex_file.write("\clearpage\n")
			if first_page:
				first_page=False
				if tp>3:
					rpp=4
				else:
					rpp=6
	if k<rpp:
		tex_file.write("\end{figure}\n")
	tex_file.write("\end{document}\n")
	
	tex_file.close()
	# Latex compiling is in batchmode, i.e. it suppress almost all the ouput it produces over the terminal -interaction=batchmode
	return_value = subprocess.call(['pdflatex',"-interaction=batchmode",t.replace("/","_s_")+"_report.tex"], shell=False)
	return_value = subprocess.call(['pdflatex',"-interaction=batchmode",t.replace("/","_s_")+"_report.tex"], shell=False)
	shutil.copy2(os.getcwd()+"/"+t.replace("/","_s_")+"_report.pdf", retval)
	os.chdir(retval)
	shutil.rmtree("tmp")

	

	

import numpy as np
import math
from scipy.optimize import curve_fit
def fit_func(x,a,b,c):
	return a+b*np.power(x,c)
	
def calc_fitVal(x,y):
	popt,pcov= curve_fit(fit_func,x,y)
	return popt, pcov



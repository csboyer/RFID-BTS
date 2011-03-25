from scipy import *
from scipy import fftpack
import array
import Gnuplot
import math
import os

# plots the data from filename.  The file must contain all floats.
def plot_file(filename):
	gp = Gnuplot.Gnuplot(persist=1)
	gp2 = Gnuplot.Gnuplot(persist=1)
	#The following commented code is to output to gnuplot
	data = array.array('f')
	outputF = open(filename)
	data.fromfile(outputF, 10)
	gp.title('Frequency Plot')
	gp('set style data lines')
	gp('set xlabel "Frequency"')
	gp('set ylabel "Magnitude"')
	gp.plot(abs(fft(data)))
	data2 = Gnuplot.Data(data, title='Time plot')
	gp2('set xlabel "Time (samples)"')
	gp2('set ylabel "Magnitude"')	
	gp2('set yrange [0:2]')
	gp2('set style data lines')
	gp2.plot(data2)
	
plot_file('output.dat')

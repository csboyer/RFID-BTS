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
	num_samples = 60000

	#The following commented code is to output to gnuplot
	data = array.array('f')
	outputF = open(filename)
	data.fromfile(outputF, num_samples)
	gp.title('Frequency Plot')
	gp('set style data lines')
	gp('set xlabel "Frequency"')
	gp('set ylabel "Magnitude"')
	fftout = abs(fft(data))
	norm = [fftout[i] / fftout[0] for i in range(num_samples / 2)]
	norm = 20 * log10(norm)
	x = list(range(num_samples / 2))
	time = [320e3 / (num_samples / 2) * x[i] for i in range(num_samples / 2)]
	data3 = Gnuplot.Data(list(time),norm, title='Time plot')
	#gp.plot(data3)

        shift = 30000
  
        maxval = max(data)
        data = [data[i] / maxval for i in range(num_samples)]
        newdat = [abs(data[i + shift]) for i in range(num_samples - shift)]
	data2 = Gnuplot.Data(newdat, title='Time plot')
	gp2('set xlabel "Time (samples)"')
	gp2('set ylabel "Magnitude"')	
	#gp2('set yrange [0:2]')
	gp2('set style data lines')
	gp2.plot(data2)
	
plot_file('outputrx.dat')

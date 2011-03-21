from scipy import *
from gnuradio import gr
import howto
import math
import array
import Gnuplot
import RFID-BTS
from scipy import fftpack

# Returns a vector of bytes given the command string in com
# Returns zero if the string is not found
def get_code(com):
	if com == "com1":
		return (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)
	elif com == "com2":
		return (5, 6, 7, 8)
	else:
		return 0

# Main block which hooks up all of the blocks
class my_top_block(gr.top_block):
	def __init__(self):
		gr.top_block.__init__(self)
 		#self.modulate = howto.square_ff()
		self.u = usrp_sink_c()
		self.usrp_interp = 512	
        	self.u.set_interp_rate(self.usrp_interp)
		self.u.set_mux(usrp.determine_tx_mux_value(self.u, 0))
		self.subdev = usrp.selected_subdev(self.u, usrp.pick_tx_subdevice(self.u))
        	self.subdev.set_gain(self.subdev.gain_range()[1])    # set max Tx gain
        	if not self.set_freq(915E6):
            		freq_range = self.subdev.freq_range()
            		print "Failed to set frequency to %s.  Daughterboard supports %s to %s" % (
                		eng_notation.num_to_str(options.freq),
                		eng_notation.num_to_str(freq_range[0]),
                		eng_notation.num_to_str(freq_range[1]))
            		raise SystemExit
		self.subdev.set_enable(True)
		self.modulate = rfidbts_pie_encoder(2)
		self.rep = gr.repeat(gr.sizeof_complex, tari / 2 * sps)
		self.rrc_taps = gr.firdes.root_raised_cosine(1, sps, tari, .02, 41) 
		self.rrcosfilt = gr.fir_filter_fff(1, self.rrc_taps)
		self.connect (self.modulate, self.rep, self.rrcosfilt, self.u)
	def set_freq(self, target_freq):
		r = self.u.tune(self.subdev.which(), self.subdev, target_freq)
		if r:
		    print "r.baseband_freq =", eng_notation.num_to_str(r.baseband_freq)
		    print "r.dxc_freq      =", eng_notation.num_to_str(r.dxc_freq)
		    print "r.residual_freq =", eng_notation.num_to_str(r.residual_freq)
		    print "r.inverted      =", r.inverted
		    # Could use residual_freq in s/w freq translator
		    return True
		return False

com = "nothing"


tb = my_top_block()
pwr_on = False
data = array.array('f')
gp = Gnuplot.Gnuplot(persist=1)

while com != "exit":

	com = raw_input("Command: ")
	if com == "start":
		pwr_on = True
		tb.start()
		print("Sending power to device.")
		#The following commented code is to output to gnuplot		
		#tb.wait()
		#outputF = open('output.dat')
		#data.fromfile(outputF, 100)
		#x = range(5000)
		#gp('set xlabel "Time (samples)"')
		#gp('set ylabel "Magnitude"')
		#gp.plot(abs(fft(tb.rrc_taps)))
		#gp.plot(abs(fft(data)))
	elif com == "stop":
		tb.stop()
		tb.wait()
		pwr_on = False
		print("Stopped sending power.")
	elif com != "exit":
		code = get_code(com)
		# If the command is ok then output the bytes associated with it to the modulating block
		# This works if the power is off or on.
		if code != 0:
			tb.modulate.send_command(code)
			print("Command sent")		
		else:
			print("Uknown command.")
	

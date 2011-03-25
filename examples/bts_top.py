#!/usr/bin/env python
#add some GPL3/GNURadio license stuff here
from scipy import *
from scipy import fftpack
import array
import Gnuplot
import math
from gnuradio import gr, eng_notation
from optparse import OptionParser
from gnuradio.eng_option import eng_option
import sys
import os
import rfidbts

class downlink_test_file_sink(gr.hier_block2):
  def __init__(self,usrp_rate,usrp_interp):
    gr.hier_block2.__init__(self,"downlink_test_file_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))
    self.c_to_f = gr.complex_to_real()
    self.rate_limiter = gr.throttle(gr.sizeof_float,usrp_rate/usrp_interp)
    self.f = gr.file_sink(gr.sizeof_float,'output.dat')

    self.connect(self,self.c_to_f,self.rate_limiter,self.f)

class downlink_usrp_sink(gr.hier_block2):
  def __init__(self,options,usrp_rate,usrp_interp,tari_rate):
    gr.hier_block2.__init__(self,"downlink_usrp_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))

    self.u = gr.usrp_sink_c()
    self.u.set_interp_rate(usrp_interp)
    self.u.set_mux(usrp.determine_tx_mux_value(self.u,0))
    self.subdev = usrp.selected_subdev(self.u,usrp.pick_tx_subdevice(self.u))
    #set max tx gain
    self.subdev.set_gain(self.subdev.gain_range()[1])
    if not self.set_freq(options.freq):
      freq_range = self.subdev.freq_range()
      print "Failed to set frequency to %s.  Daughterboard supports %s to %s" % (
            eng_notation.num_to_str(options.freq),
            eng_notation.num_to_str(freq_range[0]),
            eng_notation.num_to_str(freq_range[1]))
      raise SystemExit
    
    self.subdev.set_enable(True)

    self.connect(self,self.u)

  def set_freq(self,target_freq):
    r = self.u.tune(self.subdev.which(),self.subdev,target_freq)
    if r:
      print "r.baseband_freq =", eng_notation.num_to_str(r.baseband_freq)
      print "r.dxc_freq      =", eng_notation.num_to_str(r.dxc_freq)
      print "r.residual_freq =", eng_notation.num_to_str(r.residual_freq)
      print "r.inverted      =", r.inverted
      # Could use residual_freq in s/w freq translator
      return True
    
    return False

class bts_top_block(gr.top_block):
  def __init__(self):
    gr.top_block.__init__(self)
    (options, args) = self.get_options()
    usrp_rate = 128000000
    usrp_interp = 400
    tari_rate = 80000
    run_usrp = False

    self.downlink = rfidbts.downlink_src(tari_rate,usrp_rate/usrp_interp)
    if run_usrp:
      self.sink = downlink_usrp_sink(options,usrp_rate,usrp_interp,tari_rate)
    else:
      self.sink = downlink_test_file_sink(usrp_rate,usrp_interp)

    self.connect(self.downlink,self.sink)

  def get_options(self):
    parser = OptionParser(option_class=eng_option)

    parser.add_option("-f", "--freq", type="eng_float", default=915e6,
                      help="Select frequency (default=%default)")
		      
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
	

    return (options, args)
    
def main():
  #start executing code here
  tb = bts_top_block()
  
  try:
    tb.run()
    tb.downlink.send_pkt(get_pkt_test())
    time.sleep(0.001) #sleep for 1 millisecond
    tb.stop()

  except KeyboardInterrupt:
    pass

def get_pkt_test():
		return (0xE, 0xF, 0x0, 0xF, 0x0)

# plots the data from filename.  The file must contain all floats.
def plot_file(filename):
	gp = Gnuplot.Gnuplot(persist=1)
	gp2 = Gnuplot.Gnuplot(persist=1)
	#The following commented code is to output to gnuplot
	data = array.array('f')
	outputF = open(filename)
	data.fromfile(outputF, os.path.getsize(filename) / 4)
	
	gp('set style data lines')
	gp('set xlabel "Frequency"')
	gp('set ylabel "Magnitude"')
	gp.plot(abs(fft(data)))
	data2 = Gnuplot.Data(data, title='Plotting from Python')
	gp2('set xlabel "Time (samples)"')
	gp2('set ylabel "Magnitude"')	
	gp2('set yrange [0:2]')
	gp2('set style data lines')
	gp2.plot(data2)

def to_add():

  while com != "exit":

    com = raw_input("Command: ")
    if com == "start":
		pwr_on = True
		tb.start()
		print("Sending power to device.")
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
	
#Go to main function
if __name__ == "__main__":
  main()




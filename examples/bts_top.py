#!/usr/bin/env python
#add some GPL3/GNURadio license stuff here
from gnuradio import gr, eng_notation
from optparse import OptionParser
from gnuradio.eng_option import eng_option
import sys

import rfidbts

class bts_top_block(gr.top_block):
  def __init__(self):
    gr.top_block.__init__(self)
    (options, args) = self.get_options()
    usrp_rate = 128000000
    usrp_interp = 400
    tari_rate = 80000
    self.downlink = rfidbts.downlink_src(tari_rate,usrp_rate/usrp_interp)

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
    
    self.connect(self.downlink,self.u)

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
  except KeyboardInterrupt:
    pass

def get_code(com):
	if com == "com1":
		return (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)
	elif com == "com2":
		return (5, 6, 7, 8)
	else:
		return 0

def to_add():

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
	
#Go to main function
if __name__ == "__main__":
  main()




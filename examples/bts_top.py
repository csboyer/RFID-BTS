#!/usr/bin/env python
#add some GPL3/GNURadio license stuff here
from scipy import *
import array
import Gnuplot
import math
import sys
import os
import time


from gnuradio import gr, eng_notation
from gnuradio import usrp
from gnuradio.eng_option import eng_option
from optparse import OptionParser

import rfidbts

class downlink_test_file_sink(gr.hier_block2):
  def __init__(self,usrp_rate,usrp_interp):
    gr.hier_block2.__init__(self,"downlink_test_file_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))
    self.c_to_f = gr.complex_to_real()
    self.chop = gr.head(gr.sizeof_float, 350)
    self.f = gr.file_sink(gr.sizeof_float,'output.dat')

    self.connect(self,self.c_to_f, self.chop,self.f)

# Recieve path that inputs from the usrp at the same frequency that the other side is 
# transmitting
class recieve_path(gr.hier_block2):
  def __init__(self, options): 
    gr.hier_block2.__init__(self, "transmit_path", 
                             gr.io_signature(0, 0, 0), 
                             gr.io_signature(0, 0, 0))
    #Set up usrp block
    self.u = usrp.source_c()
    adc_rate = self.u.adc_rate()                # 64 MS/s
    usrp_decim = 16
    gain = 65
    self.u.set_decim_rate(usrp_decim)
    usrp_rate = adc_rate / usrp_decim           #  320 kS/s
    print usrp_rate
    #subdev_spec = usrp.pick_subdev(self.u)
    subdev_spec = (0,0)
    mux_value = usrp.determine_rx_mux_value(self.u, subdev_spec)
    self.u.set_mux(mux_value)
    self.subdev = usrp.selected_subdev(self.u, subdev_spec)
    self.subdev.set_gain(gain)
    self.subdev.set_auto_tr(False)
    self.subdev.set_enable(True)

    if not(self.set_freq(915e6)):	
      print "Failed to set initial frequency"

    #set up the rest of the path
    #skip = 600000
    skip = 420000
    total = 3000000
    sw_dec = 5
    #self.c_to_f = gr.complex_to_real()
    self.skip = gr.skiphead (gr.sizeof_gr_complex, skip)
    self.chop = gr.head(gr.sizeof_gr_complex, total)
    self.f = gr.file_sink(gr.sizeof_gr_complex,'outputrx1.dat')
    self.f2 = gr.file_sink(gr.sizeof_float,'outputrx2.dat')
    self.complex_to_angle = gr.complex_to_arg()
    num_taps = int(64000 / ( (usrp_decim * 4) * 40 )) #Filter matched to 1/4 of the 40 kHz tag cycle
    taps = [complex(1,1)] * num_taps
    matched_filt = gr.fir_filter_ccc(sw_dec, taps);  
    agc = gr.agc2_cc(0.3, 1e-3, 1, 1, 100) 

    self.connect(self.u, self.skip, self.chop, self.f)
    self.connect(self.chop, self.complex_to_angle, self.f2)

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

# Transmit path to talk to the rfid tag
class transmit_path(gr.hier_block2):
  def __init__(self, options): 
    gr.hier_block2.__init__(self, "transmit_path", 
                             gr.io_signature(0, 0, 0), 
                             gr.io_signature(0, 0, 0))

    # constants for usrp
    usrp_rate = 128000000
    usrp_interp = 256
    tari_rate = 40000
    gain = 5000

    run_usrp = True

    testit = False
    if testit:
      self.downlink = gr.file_source(gr.sizeof_gr_complex, "readout.dat", True)
    else:
      self.downlink = rfidbts.downlink_src(tari_rate,usrp_rate/usrp_interp)

    if run_usrp:
      self.sink = downlink_usrp_sink(options,usrp_rate,usrp_interp,tari_rate)
    else:
      self.sink = downlink_test_file_sink(usrp_rate,usrp_interp)



    self.gain = gr.multiply_const_cc(gain)

    self.connect(self.downlink, self.gain, self.sink)

# Connects both the transmit and recieve paths
class bts_top_block2(gr.top_block):
  def __init__(self):
    (options, args) = self.get_options()
    gr.top_block.__init__(self)

    self.tx_path = transmit_path(options)
    self.rx_path = recieve_path(options)
    self.connect(self.tx_path)
    self.connect(self.rx_path)

  def get_options(self):
    parser = OptionParser(option_class=eng_option)

    parser.add_option("-f", "--freq", type="eng_float", default=915e6,
                      help="Select frequency (default=%default)")
		      
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
	

    return (options, args)

# downlink block.  Contains the usrp block in a single package
class downlink_usrp_sink(gr.hier_block2):
  def __init__(self,options,usrp_rate,usrp_interp,tari_rate):
    gr.hier_block2.__init__(self,"downlink_usrp_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))

    self.u = usrp.sink_c()

    #setup daughter boards
    #subdev_spec = usrp.pick_tx_subdevice(self.u)
    subdev_spec = (1, 0)
    self.subdev = usrp.selected_subdev(self.u,subdev_spec)
    self.u.set_mux(usrp.determine_tx_mux_value(self.u,subdev_spec))
    print "Using TX d'board %s" % (self.subdev.side_and_name(),)

    #set interp rate
    self.u.set_interp_rate(usrp_interp)
    self.subdev.set_gain(self.subdev.gain_range()[2])
    
    #setup frequency
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

    
def main():
  #start executing code here
  tb = bts_top_block2()

  tb.start()
  
  #Initially send something to the tag
  time.sleep(.1)  
  code = get_code("q")
  tb.tx_path.downlink.send_pkt(code)

  control_loop(tb)
  tb.stop()

# Returns a vector of bytes given the command string in com and the number of bits to transmit
# Returns zero if the string is not found
def get_code(com):
	if com[0] == "s":
		if len(com) > 2:
			return ("4" + com[2:len(com)])
		else:
			return 0
	if com == "q":
		return ("410000101000001111")
	if com == "q2":
		return ([0x4, 0x8, 0x5, 0x0, 0x7, 0xF, 0x3], 22)
	if com == "q1":
		return ([0x4, 0x8, 0x6, 0x0, 0x0, 0x1, 0xD], 22)
	elif com == "loop test":
		a = [0x4, 1, 0]
		return ([0x5, 0x8, 0x6, 0x0, 0x0, 0x8,0x0], 22)
	else:
		return 0

# Controls the program flow.  Enter commands here.
def control_loop(tb):
  pwr_on = True
  com = "nothing"
  while com != "e":
    com = raw_input("Command: ")
    if com == "start":
      if pwr_on == False:	
        pwr_on = True
        tb.start()
	print("Sending power to device.")
      else:
        print "Power already on."
    elif com == "stop":
      if pwr_on == true:
        tb.stop()
        tb.wait()
        pwr_on = False
        print("Stopped sending power.")
      else:
        print "Power not on"
    elif com == "loop":
      x = 0
      while x != 12:
        if not tb.tx_path.downlink.pie_encoder.msgq().full_p():
          code = get_code("q")
          tb.tx_path.downlink.send_pkt(code)
    elif com != "e":
      code = get_code(com)
		# If the command is ok then output the bytes associated with it to the modulating block
		# This works if the power is off or on.
      if code != 0:
        tb.tx_path.downlink.send_pkt(code)
        print("Command sent")		
      else:
        print("Uknown command.")
	
#Go to main function
if __name__ == "__main__":
  main()


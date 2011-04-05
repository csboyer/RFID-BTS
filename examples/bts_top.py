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
    usrp_decim = 200
    gain = 65
    self.u.set_decim_rate(usrp_decim)
    usrp_rate = adc_rate / usrp_decim           #  320 kS/s
    subdev_spec = usrp.pick_subdev(self.u)
    mux_value = usrp.determine_rx_mux_value(self.u, subdev_spec)
    self.u.set_mux(mux_value)
    self.subdev = usrp.selected_subdev(self.u, subdev_spec)
    self.subdev.set_gain(gain)
    if not(self.set_freq(915e6)):	
      self._set_status_msg("Failed to set initial frequency")

    #set up the rest of the path
    self.c_to_f = gr.complex_to_real()
    self.skip = gr.skiphead (gr.sizeof_float, 30000)
    self.chop = gr.head(gr.sizeof_float, 200000)
    self.f = gr.file_sink(gr.sizeof_float,'outputrx.dat')

    self.connect(self.u, self.c_to_f, self.skip, self.chop, self.f)

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
    usrp_interp = 400
    tari_rate = 40000
    gain = 10000
    run_usrp = True
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

class downlink_usrp_sink(gr.hier_block2):
  def __init__(self,options,usrp_rate,usrp_interp,tari_rate):
    gr.hier_block2.__init__(self,"downlink_usrp_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))

    self.u = usrp.sink_c()

    #setup daughter boards
    subdev_spec = usrp.pick_tx_subdevice(self.u)
    self.subdev = usrp.selected_subdev(self.u,subdev_spec)
    self.u.set_mux(usrp.determine_tx_mux_value(self.u,subdev_spec))
    print "Using TX d'board %s" % (self.subdev.side_and_name(),)

    #set interp rate
    self.u.set_interp_rate(usrp_interp)

    #set max tx gain
    self.subdev.set_gain(self.subdev.gain_range()[1])
    
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

class bts_top_block(gr.top_block):
  def __init__(self):
    gr.top_block.__init__(self)
    (options, args) = self.get_options()
    usrp_rate = 128000000
    usrp_interp = 400
    tari_rate = 40000
    gain = 10000
    run_usrp = True

    self.downlink = rfidbts.downlink_src(tari_rate,usrp_rate/usrp_interp)
    if run_usrp:
      self.sink = downlink_usrp_sink(options,usrp_rate,usrp_interp,tari_rate)
    else:
      self.sink = downlink_test_file_sink(usrp_rate,usrp_interp)

    self.gain = gr.multiply_const_cc(gain)

    self.connect(self.downlink, self.gain, self.sink)

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
  tb = bts_top_block2()

  tb.start()
  time.sleep(.005)  

  #Initially send something to the tag
  code = get_code("query")
  tb.tx_path.downlink.send_pkt(code[0], code[1])

  control_loop(tb)
  tb.stop()

# Returns a vector of bytes given the command string in com and the number of bits to transmit
# Returns zero if the string is not found
def get_code(com):
	if com == "select":
		return ([0x4, 0xA, 0x6, 0x0, 0x0, 0x8, 0x0], 22)
	if com == "query":
		return ([0x5, 0x8, 0x6, 0x0, 0x0, 0x8, 0x0], 22)
	elif com == "loop test":
		a = [0x4, 1, 0]
		return ([0x5, 0x8, 0x6, 0x0, 0x0, 0x8,0x0], 22)
	else:
		return 0

# Controls the program flow.  Enter commands here.
def control_loop(tb):
  pwr_on = True
  com = "nothing"
  while com != "exit":
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
          code = get_code("query")
          tb.tx_path.downlink.send_pkt(code[0], code[1])
    elif com != "exit":
      code = get_code(com)
		# If the command is ok then output the bytes associated with it to the modulating block
		# This works if the power is off or on.
      if code != 0:
        tb.tx_path.downlink.send_pkt(code[0], code[1])
        print("Command sent")		
      else:
        print("Uknown command.")
	
#Go to main function
if __name__ == "__main__":
  main()


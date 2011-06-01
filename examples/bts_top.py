#!/usr/bin/env python
#add some GPL3/GNURadio license stuff here
from gnuradio import gr, eng_notation, usrp
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import howto
import rfidbts
import time

class downlink_test_file_sink(gr.hier_block2):
  def __init__(self,usrp_rate,usrp_interp):
    gr.hier_block2.__init__(self,"downlink_test_file_sink",
                            gr.io_signature(1,1,gr.sizeof_gr_complex),
                            gr.io_signature(0,0,0))
    self.c_to_f = gr.complex_to_real()
    self.chop = gr.head(gr.sizeof_float, 350)
    self.f = gr.file_sink(gr.sizeof_float,'output.dat')

    self.connect(self,self.c_to_f, self.chop,self.f)

class dc_block(gr.hier_block2):
    def __init__(self,avg_len):
        gr.hier_block2.__init__(
                self, 
                "dc_block",
                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                gr.io_signature(1, 1, gr.sizeof_gr_complex))

        self.mvg_avg = gr.moving_average_cc(
                avg_len, 
                complex(-1.0,0.0) / avg_len, 
                4 * avg_len)
        self.adder = gr.add_cc()

        self.connect(
                self, 
                self.mvg_avg, 
                (self.adder,0))
        self.connect(
                self, 
                (self.adder,1))
        self.connect(
                self.adder,
                self)

class symbols_decoder(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(
                self, 
                "dc_block",
                gr.io_signature(1, 1, gr.sizeof_float),
                gr.io_signature(1, 1, gr.sizeof_float))
        match0 = [1, 1]
        match1 = [1, -1]
        self.match_filt0 = gr.fir_filter_fff(1, match0)
        self.match_filt1 = gr.fir_filter_fff(1, match1)
        self.destroy0 = gr.keep_one_in_n(gr.sizeof_float, 2)
        self.destroy1 = gr.keep_one_in_n(gr.sizeof_float, 2)
        self.compare = rfidbts.compare()
        #(self.compare, 0),
        self.connect(self, self.match_filt0, self.destroy0, (self.compare, 0), self)
        self.connect(self, self.match_filt1, self.destroy1, (self.compare, 1))

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
    deci = 1

    self.agc = gr.agc_cc( rate = 1e-7, reference = 1.0, gain = 0.001, max_gain = 0.5)

    matchtaps = [complex(-1,-1)] * 8 + [complex(1,1)] * 8 + [complex(-1,-1)]* 8 + [complex(1,1)]* 8
    #matchtaps = [complex(-1,-1)] * 8 + [complex(1,1)] * 8 
    self.matchfilter = gr.fir_filter_ccc(1, matchtaps)

    reverse = [complex(1,1)] * (8 / deci) + [complex(-1,-1)] * (8 / deci) + [complex(1,1)]* (8 / deci) + [complex(-1,-1)]* (8 / deci)
    #pretaps = matchtaps * 3 + reverse * 4 + matchtaps * 4 + reverse * 8 + matchtaps * 6 + matchtaps * 62
    pretaps = matchtaps * 2 + reverse * 2 + matchtaps * 2 + reverse * 4 + matchtaps * 3 + matchtaps * 31
    #pretaps =  matchtaps * 3 + reverse * 8 + matchtaps * 6 + matchtaps * 64
    self.preamble_filter = gr.fir_filter_ccc(1, pretaps)

    self.c_f = gr.complex_to_real()
    self.c_f2 = gr.complex_to_real()

    self.lock = howto.lock_time(32, 5, 32)

    self.pd = howto.find_pre_ff(55, 200)

    self.dec = symbols_decoder()

    self.vect = gr.vector_sink_f()
    
    #self.connect(self.u, self.agc, self.matchfilter, self.c_f, (self.lock, 0),  self.dec, self.vect)
    #self.connect(self.agc, self.preamble_filter, self.c_f2, self.pd, (self.lock, 1))
    self.connect(self.u, self.agc, self.matchfilter, self.c_f, self.vect)

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
    usrp_interp = 200
    tari_rate = 40000
    gain = 5000

    run_usrp = True

    testit = False
    if testit:
      self.downlink = gr.file_source(gr.sizeof_gr_complex, "readout.dat", True)
    else:
      self.downlink = rfidbts.downlink_src(
                samp_per_delimiter = 8, 
                samp_per_cw = 64 * 16 * 60,
                samp_per_wait = 64 * 16 * 4,
                samp_per_tari = 16 ,
                samp_per_pw = 8,
                samp_per_trcal = 53 ,
                samp_per_data1 = 32)

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
  tb.tx_path.downlink.send_pkt_preamble(code)
  #tb.tx_path.downlink.send_pkt(code2)
  #for i in range(10):
  #    tb.tx_path.downlink.send_pkt(code)
  #    tb.tx_path.downlink.send_pkt(code2)
  #    tb.tx_path.downlink.send_pkt(code2)

  control_loop(tb)
  tb.stop()

# given a string of bit inputs makes a crc of command
def make3_crc_5(bit_stream):
    preset = "01001"
    register = "01001"
    flag = True
    for i in range(len(bit_stream)):
        if register[0] == bit_stream[0]:
            flag = False
        else:
            flag = True
        register = register[1:] + "0"
        bit_stream = bit_stream[1:]
        if flag:
            register = string_xor(register, preset)
    return register

# xor on two equal length strings of bit representations
def string_xor(str1, str2):
    out = ""
    for i in range(len(str1)):
        if str1[i] != str2[i]:
            out = out[:(i)] + '1' + out[(i+1):]
        else:
            out = out[:(i)] + '0' + out[(i+1):]   
    return out

# Returns a vector of bytes given the command string in com and the number of bits to transmit
# Returns zero if the string is not found
def get_code(com):
	if com[0] == "s":
		if len(com) > 2:
			return ("4" + com[2:len(com)])
		else:
			return 0
	if com == "q":
		stream = (1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0)
		return (stream)
		#return stream
	if com == "ak":
		return ("5010000000000000111")
	if com == "qr":
		return ("50001")
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
        tb.tx_path.downlink.send_pkt_preamble(code)
        time.sleep(.1) 
        print("Command sent")
        print tb.rx_path.vect.data() 	
        tb.rx_path.vect.clear()	
      else:
        print("Uknown command.")
	
#Go to main function
if __name__ == "__main__":
  main()


from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option

class bts_downlink_block(gr.hier_block2):
  def __init__(self,usrp_sample_rate,tari_time):
    gr.hier_block2.__init__(self, "bts_downlink_block",
                            gr.io_signature(0, 0, 0), #input is command to send to tag
                            gr.io_signature(1, 1, gr.sizeof_gr_complex)) #output is complex baseband to sink
    
    

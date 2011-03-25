from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option
import rfidbts_swig as rfidbts


class downlink_src(gr.hier_block2):
  def __init__(self,tari_rate,usrp_rate):
    gr.hier_block2.__init__(self, "downlink_src",
                            gr.io_signature(0, 0, 0), #input is command to send to tag
                            gr.io_signature(1, 1, gr.sizeof_gr_complex)) #output is complex baseband to sink
    #declare pie encoder
    self.pie_encoder = rfidbts.pie_encoder(2)
    #construct rrc filter
    #each half tari is a symbol
    samples_per_symbol = usrp_rate / tari_rate / 2
    gain = samples_per_symbol
    symbol_rate = 1.0
    alpha = 0.02
    ntaps = 41
    self.rrc_taps = gr.firdes.root_raised_cosine(gain,samples_per_symbol,symbol_rate,alpha,ntaps)
    #construct interpolator
    interp_factor = samples_per_symbol
    self.rrc_interpolator = gr.interp_fir_filter_ccf(interp_factor, self.rrc_taps)

    #connect everything together
    self.connect(self.pie_encoder, self.rrc_interpolator, self)

  def send_pkt(self, msg):
    #a pkt consists of a list of hex values. Want to conver it to a string of bits!
    #get the frame/preamble flag from the front of the packet. 
    #bitize everything else
    bit_chunks = chr(msg.popleft())
    for byte in msg:
      byte_str = self.bin(byte)
      for bit in byte_str:
        bit_chunks = bit_chunks + chr(int(bit))
    #add bit chunks to queue
    self.pie_encoder.msgq().insert_tail(gr.make_message_from_string(bit_chunks))
    
  def bin(s):
    return str(s) if s<=1 else bin(s>>1) + str(s&1)


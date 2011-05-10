from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option
import rfidbts_swig as rfidbts
import Gnuplot

class packet:
  def __init__(self, data, numbits):
    self.numbits = numbits
    self.data = data

  def bin(self,s):
    temp = ""
    count = 1
    for i in range(4):
      if self.numbits > 0:
        if count & s != 0:
          temp = "1" + temp
        else:
          temp = "0" + temp
        count = count * 2
        self.numbits = self.numbits - 1
    return temp

  def to_bits(self):
    #a pkt consists of a list of hex values. Want to conver it to a string of bits!
    #get the frame/preamble flag from the front of the packet. 
    #bitize everything else
    bit_chunks = str(self.data.pop(0))
    for byte in self.data:
      byte_str = self.bin(byte)
      for bit in byte_str:
        bit_chunks = bit_chunks + str(int(bit))
    return bit_chunks

def make_crc_5(bit_stream):
    bit_loc = 1 << 31
    poly = 0x29
    polylength = 6
    poly = poly << 32 - polylength
    bit_stream = bit_stream << 5
    for i in range(27):
        if bit_loc & bit_stream:
            bit_stream = bit_stream ^ poly
        poly = poly >> 1
        bit_loc = bit_loc >> 1
    return bit_stream


def make2_crc_5(bit_stream):
    bit_stream2 = bit_stream + "00000"
    poly = "101001"
    for i in range(len(bit_stream)):
        if bit_stream2[0] == "1":
            bit_stream2 = string_xor(bit_stream2[0:6], poly) + bit_stream2[6:]
        bit_stream2 = bit_stream2[1:]
    return bit_stream2

def string_xor(str1, str2):
    out = ""
    for i in range(len(str1)):
        if str1[i] != str2[i]:
            out = out[:(i)] + '1' + out[(i+1):]
        else:
            out = out[:(i)] + '0' + out[(i+1):]   
    return out


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
    alpha = 0.1
    ntaps = 8 * samples_per_symbol
    #self.rrc_taps = gr.firdes.root_raised_cosine(gain,samples_per_symbol,symbol_rate,alpha,ntaps)
    self.rrc_taps = [1] * samples_per_symbol

    #construct interpolator
    interp_factor = samples_per_symbol
    self.rrc_interpolator = gr.interp_fir_filter_ccf(interp_factor, self.rrc_taps)
    #self.rrc_interpolator = gr.interp_fir_filter_ccf(1, self.rrc_taps)
    #self.stretch = gr.repeat(gr.sizeof_gr_complex, interp_factor)
    #connect everything together
    self.connect(self.pie_encoder, self.rrc_interpolator, self)
    #self.connect(self.pie_encoder, self.stretch, self.rrc_interpolator, self)


  def send_pkt(self, msg):
    #add bit chunks to queue
    self.pie_encoder.msgq().insert_tail(gr.message_from_string(msg + make2_crc_5(msg)))


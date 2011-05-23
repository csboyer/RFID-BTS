from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option
import rfidbts_swig as rfidbts

from bitarray import bitarray
from crc_algorithms import Crc

class downlink_src(gr.hier_block2):
  def __init__(
          self,
          samp_per_delimiter, #seconds for delimiter
          samp_per_cw,        #seconds for bootup time
          samp_per_wait,      #seconds to keep tag powered
          samp_per_tari,      #seconds per tari
          samp_per_pw,        #length of pw in tari
          samp_per_trcal,     #length of data 1 symbol in taris
          samp_per_data1):    #length of trcal in taris

    gr.hier_block2.__init__(self, "downlink_src",
                            gr.io_signature(0, 0, 0), #nothing
                            gr.io_signature(1, 1, gr.sizeof_gr_complex)) #output is complex baseband to sink

    self.pie_encoder = rfidbts.pie_encoder(
            samp_per_delimiter,
            samp_per_tari,
            samp_per_pw,
            samp_per_trcal,
            samp_per_data1,
            samp_per_cw,
            samp_per_wait)

    self.connect(
            self.pie_encoder, 
            self)

    self.crc = Crc(
            width = 5, 
            poly = 0x9,
            reflect_in = True,
            xor_in = 0xA,
            reflect_out = True,
            xor_out = 0x0)

  def send_pkt_preamble(self, msg):
      #      msg.append(0)
      #msg.append(0)
      #msg.append(0)
      #msg.append(0)
      #msg.append(0)
      crc = (0,0,0,0,0)
      print 'Sending pkt with preamble: ', msg, 'CRC: ', crc
      self.pie_encoder.snd_frame_preamble(msg + crc)

  def send_pkt_framesync(self, msg):
      msg.append(0)
      msg.append(0)
      msg.append(0)
      msg.append(0)
      msg.append(0)

      self.pie_encoder.snd_frame_framesync(msg.to01())

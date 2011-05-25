from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option
import rfidbts_swig as rfidbts

from bitarray import bitarray
from crc_algorithms import Crc

def string_xor(str1, str2):
    out = ""
    for i in range(len(str1)):
        if str1[i] != str2[i]:
            out = out[:(i)] + '1' + out[(i+1):]
        else:
            out = out[:(i)] + '0' + out[(i+1):]   
    return out

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

  def send_pkt_preamble(self, msg):
      #      msg.append(0)
      #msg.append(0)
      #msg.append(0)
      #msg.append(0)
      #msg.append(0)
      msgs = str(msg)
      msgs = msgs[1:len(msgs)-1:3]
      crc = make3_crc_5(msgs)
      print 'Sending pkt with preamble: ', msg, 'CRC: ', tuple(crc)
<<<<<<< HEAD
      self.pie_encoder.snd_frame_preamble(msg + tuple([int(i) for i in tuple(crc)]))
=======
      self.pie_encoder.snd_frame_preamble(msg + (1,1,1,1,1))
>>>>>>> 142c3608e2b5a66ae3a4f5b6381256efdb99c8b2

  def send_pkt_framesync(self, msg):
      msg.append(0)
      msg.append(0)
      msg.append(0)
      msg.append(0)
      msg.append(0)

      self.pie_encoder.snd_frame_framesync(msg.to01())

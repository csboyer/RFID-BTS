import math
from numpy import *

from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option

from bitarray import bitarray
import rfidbts

class transceiver(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(
                self,
                "bts_transceiver",
                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                gr.io_signature(1, 1, gr.sizeof_gr_complex))
        self.TX_block = rfidbts.receive_gate(
                threshold = 0.04,     #bit above the noise floor
                pw_preamble = 26,     #26 PW in downlink frame
                off_max = 15,         #should not be in the off state for more than 13 us
                mute_buffer = 15,     #wait for another wait 30us
                tag_response = 1000   #1ms
                )
        #avg over 16 taps
        self.dc_block_filt = dc_block(6)
        self.agc = gr.agc_cc(
               rate = 5e-1, 
               reference = 1.0, 
               gain = 10.0,
               max_gain = 100.0)
# tari 25us = 16 samples
# delimiter 12.5 us = 8 samples 
# data1 = 2.0 tari = 50 us = 32 samples
# RTCal = data0 + data1 = 75 us = 44 samples
# TRCal = 83.3 us = 53 samples Within a 0.3 us of target!
# PW little less than half tari = 7 samples
# CW = 1600 us = 64 * 16 sample
# Wait = 4 * CW
# This should give a baseband rate of 640KHz!
        self.tx_encoder = rfidbts.downlink_src(
                samp_per_delimiter = 8, 
                samp_per_cw = 64 * 16 * 60,
                samp_per_wait = 64 * 16 * 4,
                samp_per_tari = 16,
                samp_per_pw = 8,
                samp_per_trcal = 53,
                samp_per_data1 = 32)


        self.diag_output = gr.file_sink(
                itemsize = gr.sizeof_gr_complex,
                filename = "rcvr_out.dat")
        self.tx_block_debug = gr.file_sink(
                itemsize = gr.sizeof_gr_complex,
                filename = "tx_block.dat")

        self.connect(
                self,
                self.TX_block,
                self.diag_output)
        self.connect(
                self.tx_encoder,
                self)
    def snd_query(self):
#               *    Cmd    * DR *  M  * TRext *  Sel  * Session * Target *    Q    *
        frame = (1, 0, 0, 0,  0,  1, 0,    1,     0, 0,    0, 0,   0,       0, 0, 0,0)
        self.tx_encoder.send_pkt_preamble(frame)

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


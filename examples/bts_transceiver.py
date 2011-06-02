import math
from numpy import *

from gnuradio import gr, eng_notation
from gnuradio.eng_option import eng_option

from bitarray import bitarray
import rfidbts

# This block decodes half symbols into 0s and 1s
class symbols_decoder(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(
                self, 
                "dc_block",
                gr.io_signature(1, 1, gr.sizeof_float),
                gr.io_signature(1, 1, gr.sizeof_float))
        # set up paralell match filters
        match0 = [1, 1]
        match1 = [1, -1]
        self.match_filt0 = gr.fir_filter_fff(1, match0)
        self.match_filt1 = gr.fir_filter_fff(1, match1)
        # Decimate by two because there are two samples per symbol
        self.destroy0 = gr.keep_one_in_n(gr.sizeof_float, 2)
        self.destroy1 = gr.keep_one_in_n(gr.sizeof_float, 2)
        # Compare the two match filters
        self.compare = rfidbts.compare()
        # Connect it all
        self.connect(self, self.match_filt0, self.destroy0, (self.compare, 0), self)
        self.connect(self, self.match_filt1, self.destroy1, (self.compare, 1))

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
        #avg over 32 taps
        self.dc_block_filt = dc_block(4)
        self.agc = gr.agc_cc(
               rate = 5e0, 
               reference = 1.0, 
               gain = 300.0,
               max_gain = 500.0)
        mf = (1.0,1.0,-1.0,-1.0,1.0,1.0,-1.0,-1.0)
        self.match_filt = gr.fir_filter_ccf(1,mf)
        self.msg_queue = gr.msg_queue(100)
        self.preamble = rfidbts.preamble_det(shared_q = self.msg_queue,
                                             samples_per_frame = 985,
                                             detection_threshold = 0.0)
        self.timing_sync = rfidbts.gardner_timing_cc(0.0, 0.0, 8.0, 0.1, 1.0)
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
        
        self.msg_s = gr.message_source(gr.sizeof_gr_complex,
                                       self.msg_queue)
        self.diag_output = gr.file_sink(
                itemsize = gr.sizeof_gr_complex,
                filename = "rcvr_out.dat")
        self.tx_block_debug = gr.file_sink(
                itemsize = gr.sizeof_gr_complex,
                filename = "tx_block.dat")
        self.timing_debug = gr.file_sink(
                itemsize = gr.sizeof_float,
                filename = "timing_block.dat")

        self.connect(
                self.msg_s,
                (self.timing_sync,0),
                self.diag_output)
        self.connect(
                (self.timing_sync,1),
                self.timing_debug)

        self.connect(
                self,
                self.TX_block,
                self.match_filt,
                self.agc,
                self.preamble)
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
                1000)
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


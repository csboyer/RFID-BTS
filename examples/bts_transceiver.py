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
#TX BLOCK - Filters out the downlink
        self.TX_block = rfidbts.receive_gate(
                threshold = 0.04,     #bit above the noise floor
                pw_preamble = 26,     #26 PW in downlink frame
                off_max = 15,         #should not be in the off state for more than 13 us
                mute_buffer = 15,     #wait for another wait 30us
                tag_response = 1000   #1ms
                )
#AGC
        self.agc = gr.agc_cc(
               rate = 5e0, 
               reference = 1.0, 
               gain = 300.0,
               max_gain = 500.0)
#Match Filter - 4 samples per chip
        mf = (1.0,1.0,-1.0,-1.0,1.0,1.0,-1.0,-1.0)
        self.match_filt = gr.fir_filter_ccf(1,mf)
#Preamble Detector and queue
        self.detected_frames = gr.msg_queue(100)
        self.preamble = rfidbts.preamble_det(shared_q = self.detected_frames,
                                             samples_per_frame = 985,
                                             detection_threshold = 0.1)
#Timing synchronization
        self.timing_sync = rfidbts.gardner_timing_cc(mu = 0.0,
                                                     omega = 8.0,
                                                     omega_relative_limit = 0.1,
                                                     pll_gains = (0.15,0.05))
#Diff decode match filter and to mag, drop every two half symbols to get whole symbol
        self.data0_detector = gr.fir_filter_ccf(2, 
                                                (1.0, 1.0, 0.0))
        self.data1_detector = gr.fir_filter_ccf(2, 
                                                (1.0, -1.0, 0.0))
        self.to_mag0 = gr.complex_to_mag_squared()
        self.to_mag1 = gr.complex_to_mag_squared()
#Take difference of Mag to convert orthogonal to anipodal, then to bit slicer
        self.ortho_to_antipodal = gr.sub_ff()
        self.bit_slicer = gr.binary_slicer_fb()
#Hex dump for receiver output -- add this for now, change later --
        self.bit_dump = gr.file_sink(itemsize = gr.sizeof_char,
                                     filename = "decoded_msg.dat")
#TX Block
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
                samp_per_cw = 64 * 16 * 120,
                samp_per_wait = 64 * 16 * 4,
                samp_per_tari = 16,
                samp_per_pw = 8,
                samp_per_trcal = 53,
                samp_per_data1 = 32)
#Debug file sinks
        self.frame_queue = gr.message_source(gr.sizeof_gr_complex,
                                             self.detected_frames)
        self.diag_output = gr.file_sink(itemsize = gr.sizeof_gr_complex,
                                        filename = "rcvr_out.dat")
        self.tx_block_debug = gr.file_sink(itemsize = gr.sizeof_float,
                                           filename = "tx_block.dat")
        self.timing_debug = gr.file_sink(itemsize = gr.sizeof_gr_complex,
                                         filename = "timing_block.dat")
#Diag file dumps
        self.connect(
                self.frame_queue,
                self.diag_output)
        self.connect(
                (self.timing_sync,0),
                self.timing_debug),
        self.connect(
                (self.timing_sync,1),
                self.tx_block_debug)
###################################
#Connect RX BB to preamble detection
#RX -> Downlink Blanking -> Match filter -> AGC -> Preamble search
        self.connect(
                self,
                self.TX_block,
                self.match_filt,
                self.agc,
                self.preamble)
####################################
#Connect frame decoder
#Frame source (symbol aligned) -> Timing synch -> Orthgonal decoder (MF->Mag->Antipodal decode)
        self.connect(
                self.frame_queue,
                (self.timing_sync,0))

        self.connect(
                (self.timing_sync,0),
                self.data0_detector,
                self.to_mag0,
                (self.ortho_to_antipodal,1))
        self.connect(
                (self.timing_sync,0),
                self.data1_detector,
                self.to_mag1,
                (self.ortho_to_antipodal,0))

        self.connect(
                self.ortho_to_antipodal,
                self.bit_slicer,
                self.bit_dump)
       
####################################
#Connect up TX
        self.connect(
                self.tx_encoder,
                self)
    def snd_query(self):
#               *    Cmd    * DR *  M  * TRext *  Sel  * Session * Target *    Q    *
        frame = (1, 0, 0, 0,  0,  1, 0,    1,     0, 0,    0, 0,   0,       0, 0, 0,0)
        self.tx_encoder.send_pkt_preamble(frame)



import math
from numpy import *

from gnuradio import gr, blks2, eng_notation
from gnuradio.eng_option import eng_option
import gnuradio.gr.gr_threading as _threading

from bitarray import bitarray
import rfidbts
######################################
#Maps preamble synch'ed sample stream to a stream of miller half symbols 
class symbol_mapper(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "symbol_mapper",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
#half symbol match filter
        half_sym_taps = array([1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0])
#normalize
        half_sym_taps = half_sym_taps / sqrt(dot(half_sym_taps,half_sym_taps))
        self.mf = gr.fir_filter_ccf(1, half_sym_taps)
        self.timing_recovery = rfidbts.elg_timing_cc(phase_offset = 0,
                                                     samples_per_symbol = 8,
                                                     dco_gain = 0.04,
                                                     order_1_gain = 0.1,
                                                     order_2_gain = 0.01)
        q = gr.msg_queue(1000)
        self.timing_recovery.set_queue(q)
        rfidbts.cvar.rfid_mac.set_sync_queue(q)
        timing_s = gr.file_sink(gr.sizeof_gr_complex, "timing/symbols.dat")
        self.connect(self.timing_recovery, timing_s)
        self.connect(self, self.mf, self.timing_recovery, self)
###########################################
#Searches for the preamble from a sample stream
class preamble_search(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "preamble_search",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
        h_s_pos = array([1.0,1.0,-1.0,-1.0,1.0,1.0,-1.0,-1.0])
        h_s_neg = -1 * h_s_pos
#preamble taps +1 +1 +1 -1 -1 -1 -1 +1 +1 -1 -1 +1   
        preamble_taps = array([h_s_pos,
                              h_s_pos,
                              h_s_pos,
                              h_s_neg,
                              h_s_neg,
                              h_s_neg,
                              h_s_neg,
                              h_s_pos,
                              h_s_pos,
                              h_s_neg,
                              h_s_neg,
                              h_s_pos]).flatten()
#normalize
        preamble_taps = preamble_taps / sqrt(dot(preamble_taps, preamble_taps))
#auto-correlation filter, flip the taps for match filter
        corr = gr.fir_filter_ccf(1,flipud(preamble_taps))
#mag to square
        c_to_r = gr.complex_to_mag()
#peak detector
        thr = 10
        look = 20
        a = 0.01
        peak_d = gr.peak_detector2_fb(thr, look, a)
#preamble_gate
        preamble_gate = rfidbts.preamble_gate()
        preamble_srch = rfidbts.preamble_srch()
        preamble_align = rfidbts.preamble_align()
        q = gr.msg_queue(1000)
        rfidbts.cvar.rfid_mac.set_align_queue(q)
        preamble_align.set_queue(q)
#file sinks
        slicer_dicer_dump = gr.file_sink(gr.sizeof_gr_complex,  "search/slicer_dicer.dat")
        ms = gr.file_sink(gr.sizeof_float, "search/resamp_0.dat")
        ps = gr.file_sink(gr.sizeof_float, "search/peak_0.dat")
        strobe = gr.file_sink(gr.sizeof_char, "search/strobe_0.dat")
#connect everything
        self.connect(self, preamble_align, self)
        self.connect(self, preamble_gate, corr, c_to_r, peak_d, preamble_srch)
        self.connect(c_to_r, (preamble_srch,1))

        self.connect(c_to_r, ms)
        self.connect((peak_d,0), strobe)
        self.connect((peak_d,1), ps)
        self.connect(preamble_align, slicer_dicer_dump)

###################################
#Top level block
class proto_transceiver(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "proto_transceiver",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
#file sinks for debugging
        self.agc_out = gr.file_sink(itemsize = gr.sizeof_gr_complex,
                                    filename = "agc.dat")
#########################
#uplink blocks
        q_encoder = gr.msg_queue(100)
        self.tx_encoder = rfidbts.pie_encoder(samples_per_delimiter = 8, 
                                              samples_per_tari = 16,
                                              samples_per_pw = 8,
                                              samples_per_trcal = 53,
                                              samples_per_data1 = 32)
        self.tx_encoder.set_encoder_queue(q_encoder)
        rfidbts.cvar.rfid_mac.set_encoder_queue(q_encoder)
####################################
#downlink blocks
        q_blocker = gr.msg_queue(100)
        self.TX_blocker = rfidbts.receive_gate(threshold = 0.15,     #bit above the noise floor
                                               off_max = 15)         #should not be in the off state for more than 13 us
        self.TX_blocker.set_gate_queue(q_blocker)
        rfidbts.cvar.rfid_mac.set_gate_queue(q_blocker)
        self.blocker = gr.dc_blocker_cc(D = 5, 
                                        long_form = True)
        self.agc = gr.agc_cc(rate = 5e-1, 
                             reference = 0.707, 
                             gain = 100.0,
                             max_gain = 1000.0)
        self.search = preamble_search()
        self.half_symbols = symbol_mapper()
        self.decoder = rfidbts.orthogonal_decode()
        self.framer = rfidbts.packetizer()
#frame the rn16 and send to message queue
#####################################
#connect uplink
        self.connect(self.tx_encoder, self)
#####################################
#connect downlink
        self.connect(self,
                     self.TX_blocker,
                     self.blocker,
                     self.agc,
                     self.search,
                     self.half_symbols,
                     self.decoder,
                     self.framer)
        self.connect(self.agc, self.agc_out)
########################################
    def snd_query(self):
        rfidbts.cvar.rfid_mac.issue_downlink_command()



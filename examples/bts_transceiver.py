import math
from numpy import *

from gnuradio import gr, blks2, eng_notation
from gnuradio.eng_option import eng_option
import gnuradio.gr.gr_threading as _threading

from bitarray import bitarray
import rfidbts
#############################################
# Msg printout
class queue_watcher_thread(_threading.Thread):
    def __init__(self, q, callback):
        _threading.Thread.__init__(self)
        self.setDaemon(1)
        self.q = q
        self.callback = callback
        self.keep_running = True
        self.start()
        
    def run(self):
        while self.keep_running:
            s = self.q.delete_head().to_string()
            printout = "RN16 bits:"
            for c in s:
                printout = printout + " 0x" + c.encode("hex")
            print printout
#############################################
# Diff. decoder
class bit_slicer(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "bit_slicer",
                                gr.io_signature(2, 2, gr.sizeof_float),
                                gr.io_signature(1, 1, gr.sizeof_char))
        diff = gr.sub_ff()
        bpsk_slicer = gr.binary_slicer_fb()

        self.connect((self, 1), (diff, 0))
        self.connect((self, 0), (diff, 1))
        self.connect(diff,
                     bpsk_slicer,
                     self)

class symbol_1_slicer(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "symbol_1_slicer",
                                gr.io_signature(2, 2, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_float))
        diff = gr.sub_cc()
        to_mag = gr.complex_to_mag_squared()

        self.connect((self, 0), (diff, 0))
        self.connect((self, 1), (diff, 1))
        self.connect(diff, 
                     to_mag, 
                     self)

class symbol_0_slicer(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "symbol_0_slicer",
                                gr.io_signature(2, 2, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_float))
        adder = gr.add_cc()
        to_mag = gr.complex_to_mag_squared()

        self.connect((self, 0), (adder, 0))
        self.connect((self, 1), (adder, 1))
        self.connect(adder,
                     to_mag,
                     self)

class binary_diff_decoder(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "binary_diff_decoder",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_char))
        s_to_p = gr.stream_to_streams(gr.sizeof_gr_complex, 2)
        s_0_s = symbol_0_slicer()
        s_1_s = symbol_1_slicer()
        slicer = bit_slicer()
#connect serial to parallel to the symbol 0 and symbol 1 slicers
        self.connect(self, s_to_p)
        self.connect((s_to_p, 0), (s_0_s,0)) 
        self.connect((s_to_p, 1), (s_0_s,1)) 
        self.connect((s_to_p, 0), (s_1_s,0)) 
        self.connect((s_to_p, 1), (s_1_s,1))
#connect the slicers to bit slicers
        self.connect(s_0_s, (slicer, 0))
        self.connect(s_1_s, (slicer, 1))
        self.connect(slicer, self)
######################################
#Maps preamble synch'ed sample stream to a stream of miller half symbols 
class symbol_mapper(gr.hier_block2):
    def __init__(self):
        gr.hier_block2.__init__(self,
                                "symbol_mapper",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
        half_sym_taps = array([1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0])
#normalize
        half_sym_taps = half_sym_taps / sqrt(dot(half_sym_taps,half_sym_taps))
        self.mf = gr.fir_filter_ccf(1, half_sym_taps)
        self.timing_recovery = rfidbts.elg_timing_cc(phase_offset = 0,
                                                     samples_per_symbol = 8,
                                                     in_frame_size = 500,
                                                     out_frame_size = 7 + 12 + 32,
                                                     dco_gain = 0.04,
                                                     order_1_gain = 0.1,
                                                     order_2_gain = 0.01)

        timing_s = gr.file_sink(gr.sizeof_gr_complex, "timing/symbols.dat")
        self.connect(self.timing_recovery, timing_s)
        self.connect(self, self.mf, self.timing_recovery, self)
###########################################
#Searches for the preamble from a sample stream
class preamble_search(gr.hier_block2):
    def __init__(self, frame_size_rn16):
        gr.hier_block2.__init__(self,
                                "preamble_search",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(1, 1, gr.sizeof_gr_complex))
        self.frame_size_rn16 = frame_size_rn16
        self.create_resamplers(1.024,4)
        self.create_correlators(4)
        self.create_peak_d(4)
        self.msg_queue = gr.msg_queue(1000)
        self.pick_peak = rfidbts.pick_peak()
        self.pick_peak.set_msg_queue(self.msg_queue)
        self.slicer_dicer = rfidbts.mux_slice_dice(gr.sizeof_gr_complex)
        self.slicer_dicer.set_msg_queue(self.msg_queue)

        self.connect_coarse_search(4)
        self.connect(self.slicer_dicer, self)

    def create_resamplers(self, center_rate, rate_var):
        self.resamplers = [blks2.pfb_arb_resampler_ccf(rate = center_rate,
                                                       taps = None,
                                                       flt_size = 64,
                                                       atten = 70)]
        if rate_var == 0:
            return
        offset = 0.05 / rate_var
        for ii in range(rate_var):
            rr = blks2.pfb_arb_resampler_ccf(rate = center_rate + (ii + 1) * offset * center_rate,
                                             taps = self.resamplers[0]._taps,
                                             flt_size = 64,
                                             atten = 70)
            print "RR rate: ", center_rate + (ii + 1) * offset * center_rate
            self.resamplers.insert(0, rr)
            rr = blks2.pfb_arb_resampler_ccf(rate = center_rate -  (ii + 1) * offset * center_rate,
                                             taps = self.resamplers[0]._taps,
                                             flt_size = 64,
                                             atten = 70)
            self.resamplers.append(rr)
            print "RR rate: ", center_rate - (ii + 1) * offset * center_rate

    def create_correlators(self,rate_var):
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
        self.correlators = []
        for ii in range(2 * rate_var + 1):
            self.correlators.append(gr.fir_filter_ccf(1,flipud(preamble_taps)))
    def create_peak_d(self,rate_var):
        self.peak_d = []
        thr = 10
        look = 20
        a = 0.01
        for ii in range(2 * rate_var + 1):
            pd = gr.peak_detector2_fb(thr, look, a)
            self.peak_d.append(pd)
    def connect_coarse_search(self,rate_var):
        slicer_dicer_dump = gr.file_sink(gr.sizeof_gr_complex,  "search/slicer_dicer.dat")
        self.connect(self.slicer_dicer, slicer_dicer_dump)
        for ii in range(2 * rate_var + 1):
            rr = self.resamplers[ii]
            cc = self.correlators[ii]
            cm = gr.complex_to_mag()
            pd = self.peak_d[ii]
            pc = rfidbts.peak_count_stream(int(self.frame_size_rn16 * 3), 500)
#file sinks
            ms = gr.file_sink(gr.sizeof_float, "search/resamp_" + str(ii) + ".dat")
            ps = gr.file_sink(gr.sizeof_float, "search/peak_" + str(ii) + ".dat")
            strobe = gr.file_sink(gr.sizeof_char, "search/strobe_" + str(ii) + ".dat")
#system blocks
            self.connect(self, rr, cc, cm, pd, pc)
            self.connect(cm, (pc,1))
            self.connect(pc, (self.pick_peak, ii))
            self.connect(rr, (self.slicer_dicer, ii))
#file sinks
            self.connect(pd, strobe)
            self.connect((pd,1), ps)
            self.connect(cm, ms)
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
        frame_size_rn16 = int((32 + 32 + 12) * 8 + int(75*0.4*1.024))
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
        self.search = preamble_search(frame_size_rn16)
        self.half_symbols = symbol_mapper()
#        self.decoder = binary_diff_decoder()
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



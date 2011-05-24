#!/usr/bin/env python
#GPL3 Stuff

from gnuradio import gr, gru
from gnuradio import uhd
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser

import bts_transceiver

class app_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)

        xmtr_hack = False
        rcvr_hack = False
        
        self.options = self.run_parser()
        self.transceiver = bts_transceiver.transceiver()
        if rcvr_hack:
            self.src = gr.file_source(
                    itemsize = gr.sizeof_gr_complex,
                    filename = self.options.test_src,
                    repeat = False)
        else:
            self.bb_sink = gr.file_sink(
                    itemsize = gr.sizeof_gr_complex,
                    filename = 'bb_dump.dat')
            self.src = self.setup_src()
            self.connect(
                    self.src,
                    self.bb_sink)


        if xmtr_hack:
            self.snk = gr.throttle(
                    itemsize = gr.sizeof_gr_complex,
                    samples_per_sec = self.options.tx_samp_rate)
            self.fsnk = gr.file_sink(
                    itemsize = gr.sizeof_gr_complex,
                    filename = self.options.test_snk)
            self.connect(
                    self.snk, 
                    self.fsnk)

        else:
            self.snk = self.setup_snk()
            self.fsnk = gr.file_sink(
                    itemsize = gr.sizeof_gr_complex,
                    filename = self.options.test_snk)
            self.connect(
                    self.transceiver,
                    self.fsnk)

        self.connect(
                self.src, 
                self.transceiver)
        self.connect(
                self.transceiver, 
                self.snk)

#setup parser
    def run_parser(self):
        parser = OptionParser(option_class=eng_option)
        parser.add_option("", "--address", type="string", default="", help="Address of UHD device, [default=%default]")
        parser.add_option("", "--test-src", type="string", default="good_rn16_2.dat", help="Input file, [default=%default]")
        parser.add_option("", "--test-snk", type="string", default="readout.dat", help="Output file, [default=%default]")
        parser.add_option("", "--rx-samp-rate", type="eng_float", default=1e6, help="RX sample rate for UHD, [default=%default]")
        parser.add_option("", "--tx-samp-rate", type="eng_float", default=640e3, help="TX sample rate for UHD, [default=%default]")
        parser.add_option("", "--frequency", type="eng_float", default=915e6, help="TX/RX frequency, [default=%default]")
        parser.add_option("", "--rx-gain", type="eng_float", default=90, help="Gain for RX and TX, [default=%default]")
        parser.add_option("", "--tx-gain", type="eng_float", default=0, help="Gain for RX and TX, [default=%default]")
        (options, args) = parser.parse_args()
#checked for unparsed options
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)
        
        return options

#set up UHD  source
    def setup_src(self):
        u_src = uhd.usrp_source(device_addr=self.options.address,
                io_type=uhd.io_type.COMPLEX_FLOAT32,
                num_channels=1)
        rx_subdev = "A:"
        u_src.set_subdev_spec(rx_subdev)
        print "RX Subdev Spec ", rx_subdev

        u_src.set_samp_rate(self.options.rx_samp_rate)
        rx_rate = u_src.get_samp_rate()
        print "Requested RX rate: ", self.options.rx_samp_rate, "Hz"
        print "Actual RX rate: ", rx_rate, "Hz"

        if self.options.rx_gain is None:
            g = u_src.get_gain_range()
            self.options.rx_gain = float(g.start() + g.stop()) / 2
        u_src.set_gain(self.options.rx_gain, 0)
        rx_gain = u_src.get_gain()
        print "Requested RX gain: ", self.options.rx_gain, "dB"
        print "Actual RX gain: ", rx_gain, "dB"

        u_src.set_center_freq(self.options.frequency, 0)
        center_freq = u_src.get_center_freq()
        print "Requested RX frequency: ", self.options.frequency, "Hz"
        print "Actual RX frequency: ", center_freq, "Hz"

        return u_src

#setup UHD sink
    def setup_snk(self):
        u_snk = uhd.usrp_sink(device_addr=self.options.address,
                io_type=uhd.io_type.COMPLEX_FLOAT32,
                num_channels=1)

        tx_subdev = "B:"
        u_snk.set_subdev_spec(tx_subdev)
        print "TX Subdev Spec ", tx_subdev

        u_snk.set_samp_rate(self.options.tx_samp_rate)
        tx_rate = u_snk.get_samp_rate()
        print "Requested TX rate: ", self.options.tx_samp_rate, "Hz"
        print "Actual TX rate: ", tx_rate, "Hz"

        if self.options.tx_gain is None:
            g = u_snk.get_gain_range()
            print g.start(), g.stop()
            self.options.tx_gain = float(g.start() + g.stop()) / 2
        u_snk.set_gain(self.options.tx_gain, 0)
        tx_gain = u_snk.get_gain()
        print "Requested TX gain: ", self.options.tx_gain, "dB"
        print "Actual TX gain: ", tx_gain, "dB"

        u_snk.set_center_freq(self.options.frequency, 0)
        center_freq = u_snk.get_center_freq()
        print "Requested TX frequency: ", self.options.frequency, "Hz"
        print "Actual TX frequency: ", center_freq, "Hz"

        return u_snk

def main():
    tb = app_top_block()

    try:
        tb.start()
        tb.transceiver.snd_query()
        tb.wait()
    except KeyboardInterrupt:
        pass

if __name__ == '__main__':
    main()

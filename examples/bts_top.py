#!/usr/bin/env python
#add some GPL3/GNURadio license stuff here
from gnuradio import gr, eng_notation
from optparse import OptionParser
from gnuradio.eng_option import eng_option
import sys

import bts_downlink

class bts_top_block(gr.top_block):
  def __init__(self):
    gr.top_block.__init__(self)
    (options, args) = self.get_options()

        
  def get_options(self):
    parser = OptionParser(option_class=eng_option)

    parser.add_option("-f", "--freq", type="eng_float", default=915e6,
                      help="Select frequency (default=%default)")
    parser.add_option("-t", "--tari", type="eng_float", default=25e-6,
                      help="Select tari (default=%default)")
		      
    (options, args) = parser.parse_args()
    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
	
    if options.freq == None:
        print "Must supply frequency as -f or --freq"
        sys.exit(1)

    return (options, args)
    
def main():
  #start executing code here
  tb = bts_top_block()
  
  try:
    tb.run()
  except KeyboardInterrupt:
    pass

#Go to main function
if __name__ == "__main__":
  main()




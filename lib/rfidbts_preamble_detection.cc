/* -*- c++ -*- */
/*
 * Copyright 2004,2006 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rfidbits_preamble_det.h>
#include <gr_io_signature.h>


// public constructor that takes existing message queue
rfidbits_preamble_det_sptr
rfidbits_make_preamble_det()
{
  return rfidbits_preamble_det_sptr(new rfidbits_preamble_det());
}

rfidbits_preamble_det::rfidbits_preamble_detection() : 
	gr_block("detector",
	         gr_make_io_signature(2, 2, sizeof(float)),
	         gr_make_io_signature(1, 1, sizeof(float)))
{
  
  d_state = ST_MUTED;
  numbits = 0;
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(-1);
  preamble.push_back(-1);
  preamble.push_back(-1);
  preamble.push_back(-1);
  preamble.push_back(1);
  preamble.push_back(1);
  preamble.push_back(-1);
  preamble.push_back(-1);
  preamble.push_back(1);
  
  for( int i = 0; i < 16; i++) {
    buffer.push_back(0);
  }
  
}

int rfidbits_preamble_det::general_work(int noutput_items,
				     gr_vector_int &ninput_items,
				     gr_vector_const_void_star &input_items,
				     gr_vector_void_star &output_items)
{
  const float *in = (const float *) input_items[0];
  float *out = (float *) output_items[0];

  int j = 0;
  float temp = 0;

  for (int i = 0; i < noutput_items; i++) {

    // Adjust envelope based on current state
    switch(d_state) {
      case ST_MUTED:
          if(in[0][i] < 0) { // add to the preable buffer
            buffer.push_back(-1);
          } else {
            buffer.push_back(1);
          }
          buffer.pop_front();
          if(preamble == buffer)
            d_state = ST_UNMUTED;

      case ST_UNMUTED:
          if(numbits < MAXBITS) {
              if(in[0][i] > 0) temp += 2;
              if(in[0][i+1] > 0) temp += 1;
              out[j++] = temp;
              numbits++;   
              i++;
          else {
              d_state = ST_MUTED;
          }

    };
  }

  consume_each(noutput_items);  // Use all the inputs
  return j;		        // But only report outputs copied
}

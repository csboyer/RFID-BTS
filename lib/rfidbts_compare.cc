/* -*- c++ -*- */
/*
 * Copyright 2007,2010 Free Software Foundation, Inc.
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

/* This block compares input streams 0 and 1
   If zero is bigger: output 0
   If one is bigger: output 1  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rfidbts_compare.h>
#include <gr_io_signature.h>
#include <string.h>

rfidbts_compare_sptr
rfidbts_make_compare ()
{
  return gnuradio::get_initial_sptr(new rfidbts_compare ());
}

rfidbts_compare::rfidbts_compare ()
  : gr_sync_block ("find_pre",
		   gr_make_io_signature (2, 2, sizeof(float)),
		   gr_make_io_signature (1, 1, sizeof(float)))
{
}

int
rfidbts_compare::work (int noutput_items,
			    gr_vector_const_void_star &input_items,
			    gr_vector_void_star &output_items) {
  float *iptr0 = (float *) input_items[0];
  float *iptr1 = (float *) input_items[1];
  float *optr = (float *) output_items[0];
  
  assert(noutput_items >= 2);

  for (int i = 0; i < noutput_items; i++) {
    if (iptr0[i] * iptr0[i] >= iptr1[i] * iptr1[i]) {
      optr[i] = 0;
    } else {
      optr[i] = 1.0;
    }
  } // loop
    
  return noutput_items; 
}



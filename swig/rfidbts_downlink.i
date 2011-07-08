/* -*- c++ -*- */
/*
 * Copyright 2005,2011 Free Software Foundation, Inc.
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

#include "gnuradio.i"
%{
#include "rfidbts_pie_encoder.h"
%}

GR_SWIG_BLOCK_MAGIC(rfidbts,pie_encoder);
rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (int samples_per_delimiter, int samples_per_tari, int samples_per_pw, int samples_per_trcal, int samples_per_data1);

class rfidbts_pie_encoder : public gr_sync_block
{
 protected:
    rfidbts_pie_encoder (int samples_per_delimiter, int samples_per_tari, int samples_per_pw, int samples_per_trcal, int samples_per_data1);
 public:
  int general_work (int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);
void set_encoder_queue(gr_msg_queue_sptr q);
};


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


%import "gnuradio.i"
%{
#include "rfidbts_pick.h"
%}


GR_SWIG_BLOCK_MAGIC(rfidbts,pick_peak);
rfidbts_pick_peak_sptr rfidbts_make_pick_peak ();

class rfidbts_pick_peak : public gr_sync_block
{
 protected:
  rfidbits_pick_peak();
 public: 
  void set_msg_queue(gr_msg_queue_sptr q);
};

GR_SWIG_BLOCK_MAGIC(rfidbts,peak_count_stream);
rfidbts_peak_count_stream_sptr rfidbts_make_peak_count_stream (int frame_sample_len, int preamble_thresh);

class rfidbts_peak_count_stream : public gr_block
{
 protected:
  rfidbits_peak_count_stream(int frame_sample_len, int preamble_thresh);
 public: 
};


GR_SWIG_BLOCK_MAGIC(rfidbts,mux_slice_dice);
rfidbts_mux_slice_dice_sptr rfidbts_make_mux_slice_dice (int itemsize);

class rfidbts_mux_slice_dice : public gr_block
{
 protected:
  rfidbits_mux_slice_dice(int itemsize);
 public:
  void set_msg_queue(gr_msg_queue_sptr q);
};

GR_SWIG_BLOCK_MAGIC(rfidbts,packetizer);
rfidbts_packetizer_sptr rfidbts_make_packetizer ();

class rfidbts_packetizer : public gr_sync_block
{
 protected:
  rfidbits_packetizer();
};

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
#include "gnuradio_swig_bug_workaround.h" 
#include "rfidbts_gardner_timing_cc.h"
%}


GR_SWIG_BLOCK_MAGIC(rfidbts,gardner_timing_cc);
rfidbts_gardner_timing_cc_sptr rfidbts_make_gardner_timing_cc (float mu, float omega, const std::vector<float> &pll_gains, float omega_relative_limit, int in_frame_size, int out_frame_size);

class rfidbts_gardner_timing_cc : public gr_block
{
 protected:
  rfidbits_gardner_timing_cc(float mu, float omega, std::vector<float> &pll_gains, float omega_relative_limit, int in_frame_size, int out_frame_size);
};


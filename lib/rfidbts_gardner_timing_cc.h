
/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
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

#ifndef INCLUDED_RFIDBTS_GARDNER_TIMING_CC_H
#define	INCLUDED_RFIDBTS_GARDNER_TIMING_CC_H

#include <gr_block.h>
#include <gr_complex.h>
#include <gr_math.h>

class gri_mmse_fir_interpolator_cc;

class rfidbts_gardner_timing_cc;
typedef boost::shared_ptr<rfidbts_gardner_timing_cc> rfidbts_gardner_timing_cc_sptr;

// public constructor
rfidbts_gardner_timing_cc_sptr 
rfidbts_make_gardner_timing_cc (float mu, float gain_mu);

class rfidbts_gardner_timing_cc : public gr_block
{
 public:
  ~rfidbts_gardner_timing_cc ();
  void forecast(int noutput_items, gr_vector_int &ninput_items_required);
  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
  float mu() const { return d_mu;}
  float gain_mu() const { return d_gain_mu;}
  void set_verbose (bool verbose) { d_verbose = verbose; }

  void set_gain_mu (float gain_mu) { d_gain_mu = gain_mu; }
  void set_mu (float mu) { d_mu = mu; }

protected:
  rfidbts_gardner_timing_cc (float mu, float gain_mu);

 private:
  float g_val;
  float d_mu;
  float d_gain_mu;
  gri_mmse_fir_interpolator_cc 	*d_interp;
  bool			        d_verbose;

  gr_complex                    d_p_2T;
  gr_complex                    d_p_1T;
  gr_complex                    d_p_0T;

  friend rfidbts_gardner_timing_cc_sptr
  rfidbts_make_gardner_timing_cc (float mu, float gain_mu);
};

#endif

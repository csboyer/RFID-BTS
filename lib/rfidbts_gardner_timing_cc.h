
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
rfidbts_make_gardner_timing_cc (float mu, float omega, const std::vector<float> &pll_gains, float omega_relative_limit, int in_frame_size, int out_frame_size);

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
  void set_verbose (bool verbose) { d_verbose = verbose; }

  void set_mu (float mu) { d_mu = mu; }
  
  void set_omega (float omega) {
      d_omega = omega;
      d_min_omega = omega*(1.0 - d_omega_relative_limit);
      d_max_omega = omega*(1.0 + d_omega_relative_limit);
      d_omega_mid = 0.5*(d_min_omega+d_max_omega);
  }

protected:
  rfidbts_gardner_timing_cc (float mu, 
                             float omega, 
                             const std::vector<float> &pll_gains,
                             float omega_relative_limit,
                             int in_frame_size,
                             int out_frame_size);

 private:

  int debug_count;
  float d_mu;
  float d_mu_init;
  float d_omega;
  float d_omega_init;
  float d_omega_gain;

  int d_in_frame_size;
  int d_out_frame_size;
  int d_in_received;
  int d_out_sent;

  int d_mid_sample;
  float d_mid_mu;
  int d_forward_sample;
  float d_forward_mu;

  float d_omega_relative_limit;
  float d_min_omega;
  float d_max_omega;
  float d_omega_mid;

  std::vector<float> d_loop_filt_gains;
  std::vector<float> d_loop_filt_states;

  gri_mmse_fir_interpolator_cc 	*d_interp;
  bool			        d_verbose;

  gr_complex                    d_p_2T;
  gr_complex                    d_p_1T;
  gr_complex                    d_p_0T;

  void update_loopfilter(float timing_error);
  int update_vco();
  float gen_output_error(const gr_complex *in);

  friend rfidbts_gardner_timing_cc_sptr
  rfidbts_make_gardner_timing_cc (float mu, 
                                  float omega, 
                                  const std::vector<float> &pll_gains,
                                  float omega_relative_limit,
                                  int in_frame_size,
                                  int out_frame_size);
};

#endif

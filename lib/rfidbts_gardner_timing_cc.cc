
/* -*- c++ -*- */
/*
 * Copyright 2005,2006,2010 Free Software Foundation, Inc.
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

#include <gr_io_signature.h>
#include <gr_prefs.h>
#include <rfidbts_gardner_timing_cc.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <cfloat>

#define GARDNER_DEBUG


#include <iostream>

using namespace std;

// Public constructor
static const int FUDGE = 16;

rfidbts_gardner_timing_cc_sptr 
rfidbts_make_gardner_timing_cc(float mu, float omega, const vector<float> &pll_gains, float omega_relative_limit)
{
  return gnuradio::get_initial_sptr(new rfidbts_gardner_timing_cc (
								    mu,
                                    omega,
                                    pll_gains,
                                    omega_relative_limit));
}

rfidbts_gardner_timing_cc::rfidbts_gardner_timing_cc (
        float mu, 
        float omega,
        const vector<float> &pll_gains,
        float omega_relative_limit)
  : gr_block ("gardner_timing_cc",
	      gr_make_io_signature2 (1, 2, sizeof (gr_complex), sizeof (char)),
	      gr_make_io_signature3 (1, 3, sizeof (gr_complex), sizeof (float), sizeof (char))),
    d_mu (mu), 
    d_omega(omega),
    d_omega_relative_limit(omega_relative_limit),
    d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("gardner_timing_cc", "verbose", false)),
    d_p_2T(0), d_p_1T(0), d_p_0T(0)
{
  float prev_gain = 1.0;

  d_omega_gain = 0.18;
  set_omega(omega);
  set_relative_rate(1 / omega);

  d_loop_filt_gains.resize(pll_gains.size());
  for(int ii = 0; ii < pll_gains.size(); ii++) {
#ifdef GARDNER_DEBUG
    cout << "Setting up gain" << ii <<": " << pll_gains[ii] << endl;
#endif
    d_loop_filt_gains[pll_gains.size() - 1 - ii] = prev_gain * pll_gains[ii];
    prev_gain = d_loop_filt_gains[pll_gains.size() - 1 - ii];
  }
  //initalize filters to state 0.0
  d_loop_filt_states.assign(pll_gains.size(),0.0);

  debug_count = 0;
}

rfidbts_gardner_timing_cc::~rfidbts_gardner_timing_cc ()
{
  delete d_interp;
}

void
rfidbts_gardner_timing_cc::forecast(int noutput_items, 
        gr_vector_int &ninput_items_required)
{
  unsigned ninputs = ninput_items_required.size();
  for (unsigned i=0; i < ninputs; i++)
    ninput_items_required[i] = noutput_items  + d_interp->ntaps() + FUDGE;
}

float
rfidbts_gardner_timing_cc::gen_output_error(const gr_complex *in) {
    float g_val;
    //save old sample and interpolate based on new mu
    d_p_0T = d_interp->interpolate(in, 
                                   d_mu);
    d_p_1T = d_interp->interpolate(in + d_mid_sample, 
                                   d_mid_mu);
    d_p_2T = d_interp->interpolate(in + d_forward_sample, 
                                   d_forward_mu);
    //calculate error
    g_val = real(d_p_1T) * (real(d_p_0T) - real(d_p_2T)) +
                 imag(d_p_1T) * (imag(d_p_0T) - imag(d_p_2T));
    //g_val = -1 * g_val;
#ifdef GARDNER_DEBUG
    cout << "Interp and Calc timing error, g_val:" << g_val  << " Mu:" << d_mu << " Cnt:" << debug_count << endl;
    debug_count++;
#endif
    //cap the error by 1.0
    g_val = gr_branchless_clip(g_val,1.0);

    //return value
    return g_val;
}

int
rfidbts_gardner_timing_cc::update_vco() {
    int sample_inc;
    vector<float>::iterator vco_in = d_loop_filt_states.end() - 1;
    //update VCO
    d_omega = d_omega + *vco_in * d_omega_gain;
    //cap
    d_omega = d_omega_mid + gr_branchless_clip(d_omega - d_omega_mid, d_max_omega - d_omega_mid);
    //roll over one symbol
    d_mu = d_mu + d_omega;
    sample_inc = (int) floor(d_mu);
    d_mu = d_mu - sample_inc;
    //calc mid point next two symbols
    d_mid_mu = d_mu + d_omega / 2;
    d_mid_sample = (int) floor(d_mid_mu);
    d_mid_mu = d_mid_mu - d_mid_sample;
    //calc 2nd symbol
    d_forward_mu = d_mu + d_omega;
    d_forward_sample = (int) floor(d_forward_mu);
    d_forward_mu = d_forward_mu - d_forward_sample;

#ifdef GARDNER_DEBUG
    cout << "Update VCO: " <<  d_omega << endl;
#endif
    return sample_inc;
}

void 
rfidbts_gardner_timing_cc::update_loopfilter(float timing_error) {
    vector<float>::iterator gain_iter = d_loop_filt_gains.begin();
    vector<float>::iterator state_iter = d_loop_filt_states.begin() + 1;
#ifdef GARDNER_DEBUG
    cout << "Updating Loopfilter" << endl;
#endif

    while(state_iter != d_loop_filt_states.end()) {
        *state_iter = *state_iter + *gain_iter * timing_error + *(state_iter - 1);

        state_iter++;
        gain_iter++;
    }
}

int
rfidbts_gardner_timing_cc::general_work (int noutput_items,
				       gr_vector_int &ninput_items,
				       gr_vector_const_void_star &input_items,
				       gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];
  float *foptr = (float *) output_items[1];

  bool write_foptr = output_items.size() >= 2;
  
  int  ii = 0;	 			// input index
  int  oo = 0;				// output index
  int  ni = ninput_items[0] - d_interp->ntaps() - FUDGE;  // don't use more input than this
  int half_point;
  int frac_half_point;


  assert(d_mu >= 0.0);
  assert(d_mu <= 1.0);
  assert(d_omega >= 1.0);

  // This loop writes the error to the second output, if it exists
  if(write_foptr) {
    while(oo < noutput_items && ii < ni) {
        //calc error and update loop filter
        update_loopfilter(gen_output_error(in + ii + 5));
        //whole samples to advance by
        ii += update_vco();

        // write sampling rate
        foptr[oo] = d_omega;
        //write output
        out[oo] = d_p_0T;
        oo++;
    }
  }
  else {
    while(oo < noutput_items && ii < ni) {
        //save old sample and interpolate based on new mu
        d_p_2T = d_p_0T;

        d_p_0T = d_interp->interpolate(&in[ii], d_mu);
        //calculate error
        g_val = real(d_p_1T) * (real(d_p_0T) - real(d_p_2T)) +
                imag(d_p_1T) * (imag(d_p_0T) - imag(d_p_2T));
        //cap the error by 1.0
        g_val = gr_branchless_clip(g_val,1.0);
        //update the error integrator
        d_omega = d_omega + d_gain_omega * g_val;
        //cap the integrator
        d_omega = d_omega_mid + gr_branchless_clip(d_omega-d_omega_mid, d_omega_relative_limit);

        //exact number of samples to advance into the future
        d_mu = d_mu + d_omega + d_gain_mu * g_val;

        //Now calculate the mid symbol point 
        half_point = (int) floor(d_mu / 2);
        frac_half_point = d_mu/2 - floor(d_mu / 2);
        //interpolate the half point
        d_p_1T = d_interp->interpolate(&in[ii + half_point], 
                                       frac_half_point);

        //whole samples to advance by
        ii = ii + (int) floor(d_mu);
      
        //fractional samples to advance
        d_mu = d_mu - floor(d_mu);
        
        //calc error and update loop filter
        update_loopfilter(gen_output_error(in + ii + 5));
        //whole samples to advance by
        ii += update_vco();

        //write output
        out[oo] = d_p_0T;
        oo++;
    }
  }
  
  cout << "in " << ii << " out " << oo;
  consume_each (ii);



  return oo;
}

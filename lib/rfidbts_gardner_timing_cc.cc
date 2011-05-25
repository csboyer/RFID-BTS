
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

//#define MU_ERROR


#include <iostream>

using namespace std;

// Public constructor
static const int FUDGE = 16;

rfidbts_gardner_timing_cc_sptr 
rfidbts_make_gardner_timing_cc(float mu, float gain_mu)
{
  return gnuradio::get_initial_sptr(new rfidbts_gardner_timing_cc (
								    mu,
								    gain_mu));
}

rfidbts_gardner_timing_cc::rfidbts_gardner_timing_cc (
        float mu, 
        float gain_mu)
  : gr_block ("gardner_timing_cc",
	      gr_make_io_signature (1, 1, sizeof (gr_complex)),
	      gr_make_io_signature (1, 2, sizeof (gr_complex))),
    d_mu (mu), 
    d_gain_mu(gain_mu), 
    d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("clock_recovery_mm_cc", "verbose", false)),
    d_p_2T(0), d_p_1T(0), d_p_0T(0)
{
  if (gain_mu <  0)
    throw std::out_of_range ("Gain must be non-negative");

  set_relative_rate (0.5);
  set_history(3);			// ensure 2 extra input sample is available
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

int
rfidbts_gardner_timing_cc::general_work (int noutput_items,
				       gr_vector_int &ninput_items,
				       gr_vector_const_void_star &input_items,
				       gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];
  gr_complex *foptr = (gr_complex *) output_items[1];

  bool write_foptr = output_items.size() >= 2;
  
  int  ii = 0;	 			// input index
  int  oo = 0;				// output index
  int  ni = ninput_items[0] - d_interp->ntaps() - FUDGE;  // don't use more input than this

  assert(d_mu >= 0.0);
  assert(d_mu <= 1.0);

  float mm_val=0;
  gr_complex u, x, y;

  // This loop writes the error to the second output, if it exists
  if(write_foptr) {
    while(oo < noutput_items && ii < ni) {
        //interpolate based on new mu
        d_p_2T = d_interp->interpolate(
                                       &in[ii + 0],
                                       d_mu);
        d_p_1T = d_interp->interpolate(
                                       &in[ii + 1],
                                       d_mu);
        d_p_0T = d_interp->interpolate(
                                       &in[ii + 2],
                                       d_mu);
        //calculate error
        g_val = real(d_p_1T) * (real(d_p_0T) - real(d_p_2T)) +
                imag(d_p_1T) * (imag(d_p_0T) - imag(d_p_2T));
#ifdef MU_ERROR
        cout << "Mu offset: " << g_val << endl;
#endif
        g_val = (g_val > 1.0 || g_val < -1.0) ? copysignf(1.0, g_val) : g_val;
        //update loop filter
        d_mu = d_mu + d_gain_mu * g_val;
        if(d_mu < 0.0) {
            d_mu = -1 * d_mu;
            d_mu = ceil(d_mu) - d_mu;
            ii += 1;
        }
        else if(d_mu > 1.0) {
            d_mu = -1 * (floor(d_mu) - d_mu);
            ii += 3;
        }
        else {
            ii += 2;
        }
        // write the error signal to the second output
        foptr[oo] = gr_complex(d_mu,0);
        out[oo] = d_p_0T;
       
        oo++;
    }
  }
  else {
    while(oo < noutput_items && ii < ni) {
        //interpolate based on new mu
        d_p_2T = d_interp->interpolate(
                                       &in[ii + 0],
                                       d_mu);
        d_p_1T = d_interp->interpolate(
                                       &in[ii + 1],
                                       d_mu);
        d_p_0T = d_interp->interpolate(
                                       &in[ii + 2],
                                       d_mu);
        //calculate error
        g_val = real(d_p_1T) * (real(d_p_0T) - real(d_p_2T)) +
                imag(d_p_1T) * (imag(d_p_0T) - imag(d_p_2T));
#ifdef MU_ERROR
        cout << "Mu offset: " << g_val << endl;
#endif
        g_val = (g_val > 1.0 || g_val < -1.0) ? copysignf(1.0, g_val) : g_val;
        //update loop filter
        d_mu = d_mu + d_gain_mu * g_val;
        if(d_mu < 0.0) {
            d_mu = -1 * d_mu;
            d_mu = ceil(d_mu) - d_mu;
            ii += 1;
        }
        else if(d_mu > 1.0) {
            d_mu = -1 * (floor(d_mu) - d_mu);
            ii += 3;
        }
        else {
            ii += 2;
        }
        // write the error signal to the second output
        out[oo] = d_p_0T;
       
        oo++;
    }
  }

  consume_each (ii);

  return oo;
}

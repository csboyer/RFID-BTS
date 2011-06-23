
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
#include <rfidbts_elg_timing_cc.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <cfloat>

//#define GARDNER_DEBUG


#include <iostream>

using namespace std;

// Public constructor
static const int FUDGE = 16;

rfidbts_elg_timing_cc_sptr rfidbts_make_elg_timing_cc (float phase_offset, 
                                                       float samples_per_symbol, 
                                                       int in_frame_size, 
                                                       int out_frame_size,
                                                       float dco_gain, 
                                                       float order_1_gain, 
                                                       float order_2_gain)
{
  return gnuradio::get_initial_sptr(new rfidbts_elg_timing_cc (phase_offset,
                                                               samples_per_symbol,
                                                               in_frame_size,
                                                               out_frame_size,
                                                               dco_gain,
                                                               order_1_gain,
                                                               order_2_gain));
}

rfidbts_elg_timing_cc::rfidbts_elg_timing_cc(float phase_offset, 
                                             float samples_per_symbol, 
                                             int in_frame_size, 
                                             int out_frame_size,
                                             float dco_gain, 
                                             float order_1_gain, 
                                             float order_2_gain)
  : gr_block ("elg_timing_cc",
	          gr_make_io_signature (1, 1, sizeof (gr_complex)),
	          gr_make_io_signature2 (1, 2, sizeof (gr_complex), sizeof (float))),
    d_po_init(phase_offset), 
    d_po(phase_offset),
    d_spb_init(samples_per_symbol),
    d_spb(samples_per_symbol),
    d_integrator_1(0.0),
    d_spb_relative_limit(0.02),
    d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("elg_timing_cc", "verbose", false)),
    d_s_late(0), d_s_0T(0), d_s_early(0),
    d_in_frame_size(in_frame_size),
    d_out_frame_size(out_frame_size),
    d_in_received(0),
    d_out_sent(0),
    d_sample_catch_up(0),
    d_dco_gain(dco_gain),
    d_order_1_gain(order_1_gain),
    d_order_2_gain(order_2_gain)
{

  set_spb(d_spb);
  set_relative_rate(1 / d_spb);
  set_history(6);

  d_early_po = d_po - 0.15;
  d_early_sample = (int) floor(d_early_po);
  d_early_po = d_early_po - d_early_sample;

  d_late_po = d_po + 0.15;
  d_late_sample = (int) floor(d_late_po);
  d_late_po = d_late_po - d_late_sample;

  debug_count = 0;

}

rfidbts_elg_timing_cc::~rfidbts_elg_timing_cc ()
{
  delete d_interp;
}

void
rfidbts_elg_timing_cc::forecast(int noutput_items, 
        gr_vector_int &ninput_items_required)
{
  unsigned ninputs = ninput_items_required.size();
  for (unsigned i=0; i < ninputs; i++)
    ninput_items_required[i] = noutput_items * (int) ceil(d_spb) + d_interp->ntaps() + 1;
}

float
rfidbts_elg_timing_cc::gen_output_error(const gr_complex *in) {
    float error;
    //save old sample and interpolate based on new mu
    d_s_early = d_interp->interpolate(in + d_early_sample, 
                                      d_early_po);
    d_s_0T = d_interp->interpolate(in, 
                                   d_po);
    d_s_late = d_interp->interpolate(in + d_late_sample, 
                                     d_late_po);
    //calculate error
    error = real(d_s_0T) * (real(d_s_late) - real(d_s_early)) +
                 imag(d_s_0T) * (imag(d_s_late) - imag(d_s_early));
#ifdef GARDNER_DEBUG
    cout << "Interp and Calc timing error:" << error << " Cnt:" << debug_count << endl;
    debug_count++;
#endif
    return error;
}

int
rfidbts_elg_timing_cc::update_dco() {
    int sample_inc;
    //cap the symbol rate
    d_spb = d_spb_mid + gr_branchless_clip(d_spb - d_spb_mid, d_max_spb - d_spb_mid);
    //roll over one symbol
    d_po = d_po + d_spb;
    sample_inc = (int) floor(d_po);
    d_po = d_po - sample_inc;
    //calc early point 1/4 sample in past
    d_early_po = d_po - 0.15;
    d_early_sample = (int) floor(d_early_po);
    d_early_po = d_early_po - d_early_sample;
    //calc late point 1/4 sample in future
    d_late_po = d_po + 0.15;
    d_late_sample = (int) floor(d_late_po);
    d_late_po = d_late_po - d_late_sample;

#ifdef GARDNER_DEBUG
    cout << "Update DCO: " <<  d_spb << endl;
#endif
    return sample_inc;
}

void 
rfidbts_elg_timing_cc::update_loopfilter(float timing_error) {
    //update the DCO first
    d_loopfilter = d_order_1_gain * timing_error + d_integrator_1;
    d_spb = d_spb + d_dco_gain * d_loopfilter;
    //loop filter integrator has a delay in it
    d_integrator_1 = d_integrator_1 + d_order_1_gain * d_order_2_gain * timing_error;
#ifdef GARDNER_DEBUG
    cout << "Loop filter: " << d_loopfilter << endl;
#endif
}

int
rfidbts_elg_timing_cc::general_work (int noutput_items,
				       gr_vector_int &ninput_items,
				       gr_vector_const_void_star &input_items,
				       gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];

  int  ii = 0;	 			// input index
  int  oo = 0;				// output index
  int  ni = ninput_items[0] - d_interp->ntaps() - 1;  // don't use more input than this
  int samples_consumed;

  //sanity checks?
  assert(d_po >= 0.0);
  assert(d_po <= 1.0);
  assert(d_spb >= 1.0);
  assert(ni >= 0);
  //

  if(d_sample_catch_up > 0) {
      int catch_up = min(d_sample_catch_up, ninput_items[0]);
      d_sample_catch_up = d_sample_catch_up - catch_up;
      consume_each(catch_up);
      return 0;
  }

  //offset by one sample so it is possible to go back in time
  in++;
  while( ii < ni && oo < noutput_items) {
      switch(d_state) {
          case INIT:
              if(d_in_received < d_init) {
                  d_in_received++;
                  ii++;
              }
              else {
                  d_state = SYMBOL_TRACK;
              }
              break;
          case SYMBOL_TRACK:
              if(d_out_sent < d_out_frame_size) {
                  //calc error and update loop filter
                  update_loopfilter(gen_output_error(in + ii));
                  //whole samples to advance by
                  samples_consumed = update_dco();
                  if(samples_consumed + ii > ni) {
                      d_sample_catch_up = samples_consumed + ii - ni;
                      samples_consumed = ii - ni;
                  }
                  ii += samples_consumed;
                  d_in_received += samples_consumed;
                  // Loop filter output
                  if(output_items.size() >= 2) {
                      float *foptr = (float*) output_items[1];
                      foptr[oo] = d_loopfilter;
                  }
                  //write output
                  out[oo] = d_s_0T;
                  oo++;
                  d_out_sent++;
              }
              else {
                  d_state = FLUSH_BUFFER;
              }
              break;
          case FLUSH_BUFFER:
              if(d_in_received < d_in_frame_size) {
                  d_in_received++;
                  ii++;
              }
              else {
                  d_in_received = 0;
                  d_state = INIT;
              }
              break;
          default:
              assert(0);
      };
  }

  assert(ii <= ninput_items[0] );
  consume_each(ii);
  return oo;
}

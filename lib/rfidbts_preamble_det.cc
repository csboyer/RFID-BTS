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

#include "rfidbts_preamble_det.h"
#include <gr_io_signature.h>
#include <stdio.h>
#include <cmath>
#include <iostream>

#include "interpolator_taps.h"

using namespace std;

// public constructor that takes existing message queue
rfidbts_preamble_det_sptr
rfidbts_make_preamble_det(float detection_threshold,
                          int samples_in_frame,
                          int samples_out_frame)
{
  return rfidbts_preamble_det_sptr(new rfidbts_preamble_det(detection_threshold,
                                                            samples_in_frame,
                                                            samples_out_frame));
}

rfidbts_preamble_det::rfidbts_preamble_det(float detection_threshold,
                                           int samples_in_frame,
                                           int samples_out_frame) : 
    gr_block("preamble_detector",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_samples_per_frame(samples_in_frame),
    d_samples_per_outframe(samples_out_frame),
    d_offset_from_preamble(12 * 8),
    d_detection_threshold(detection_threshold),
    d_sample_count(0),
    d_num_phases(10)
{
    vector<float>::iterator it;
    int len = 8;
    float half_sym[] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    float half_sym_neg[] = {-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};


    //96 samples
    d_rpreamble.resize(96);
    it = d_rpreamble.begin();
    //construct preamble half symbols = +1 +1 +1 -1 -1 -1 -1 +1 +1 -1 -1 +1
    copy(half_sym,        //+1
         half_sym + len,
         it);
    it += len;
    copy(half_sym,        //+1
         half_sym + len,
         it);
    it += len;
    copy(half_sym,       //+1
         half_sym + len,
         it);
    it += len;
    copy(half_sym_neg,   // -1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym_neg,   //-1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym_neg,   //-1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym_neg,   //-1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym,       //+1
         half_sym + len,
         it);
    it += len;
    copy(half_sym,      //+1
         half_sym + len,
         it);
    it += len;
    copy(half_sym_neg,  //-1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym_neg,  //-1
         half_sym_neg + len,
         it);
    it += len;
    copy(half_sym,      //+1
         half_sym + len,
         it);
    it += len;

    d_correlator = new gri_fir_filter_with_buffer_ccf(gr_reverse(d_rpreamble));
    d_samp_buffer = new gr_complex[d_samples_per_frame];
    d_correlation_buffer = new gr_complex[d_samples_per_frame];
    d_phase_shifted = new gr_complex*[d_num_phases];
    for(int ii = 0; ii < d_num_phases; ii++) {
        d_phase_shifted[ii] = new gr_complex[d_samples_per_frame];
    }
    d_phase_shifter = new gri_fir_filter_with_buffer_ccf(d_rpreamble);
}

rfidbts_preamble_det::~rfidbts_preamble_det() {
    delete d_correlator;
    delete d_samp_buffer;
    delete d_correlation_buffer;
    for(int ii = 0; ii < d_num_phases; ii++) {
        delete d_phase_shifted[ii];
    }
    delete d_phase_shifted;
}

int rfidbts_preamble_det::find_frame_start() {
    vector<float> t;
    float best_so_far = 0.0;
    float mag_sqr;
    const float *s,*e;
    int time_index;
    int phase_index;
    int ii,jj;
    
  for(jj = 0; jj < d_num_phases; jj++) {
    s = taps[(int) (1.0 * jj / d_num_phases * NSTEPS)];
    e = s + NTAPS;
    vector<float> t(s,e);
    d_phase_shifter->set_taps( t );
    d_phase_shifter->filterN(d_phase_shifted[jj],
                             d_samp_buffer,
                             d_samples_per_frame - NTAPS);
    d_correlator->filterN(d_correlation_buffer,
                          d_phase_shifted[jj],
                          d_samples_per_frame - d_rpreamble.size());
    for(ii = d_rpreamble.size() + 2 * 16; ii < d_samples_per_frame - d_samples_per_outframe; ii++) {
        mag_sqr = d_correlation_buffer[ii].real() * d_correlation_buffer[ii].real() +
                  d_correlation_buffer[ii].imag() * d_correlation_buffer[ii].imag();
        if(mag_sqr > best_so_far) {
            best_so_far = mag_sqr;
            time_index = ii;
            phase_index = jj;
        }
    }
  }
    memcpy(d_samp_buffer,
           d_phase_shifted[phase_index],
           sizeof(gr_complex) * d_samples_per_frame);

    if(best_so_far < d_detection_threshold) {
        time_index = -1;
    }
    time_index = time_index - 9;

    cout << "Preamble at: " << time_index  << " Phase: " << phase_index << " Corr: " << best_so_far << endl;
    time_index = time_index;
    return time_index;
}

int rfidbts_preamble_det::send_frame_samples(int req_samples, gr_complex *out) {
    gr_complex *buf;
    int buf_len;

    buf_len = min(req_samples, d_frame_end - d_frame_start);
    
     //use the preamble to lock onto timing. . .
    buf = d_samp_buffer + d_frame_start;
    memcpy(out,
           buf,
           sizeof(gr_complex) * buf_len);
    d_frame_start += buf_len;   
    return buf_len;
}

int rfidbts_preamble_det::general_work(int noutput_items,
				     gr_vector_int &ninput_items,
				     gr_vector_const_void_star &input_items,
				     gr_vector_void_star &output_items)
{
  bool loop_cond = true;
  int oo = 0;
  int ii = 0;
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];
  
  while(loop_cond) {
  loop_cond = false;

  switch(d_state) {
      case GATHER_SAMPLES:
          //gather samples to search for preamble
          while(ii < ninput_items[0]) {
            if(d_sample_count < d_samples_per_frame) {
                d_samp_buffer[d_sample_count] = in[ii];
                ii++;
                d_sample_count++;
            }
            else {
                d_state = SEARCH;
                d_sample_count = 0;
                loop_cond = true;
                break;
            }
          }
          ///
          break;
      case SEARCH:
          //find the start of the preamble
          d_frame_start = find_frame_start();
          if(d_frame_start >= 0) {
            //found preamble
            d_frame_end = d_frame_start + d_samples_per_outframe;
            d_state = OUTPUT_FRAME;
            loop_cond = true;
          }
          else {
              //did not find preamble
              d_state = GATHER_SAMPLES;
          }
          //
          break;
      case OUTPUT_FRAME:
          //copy samples to output frame
          oo = send_frame_samples(noutput_items, out);
          assert(d_frame_start <= d_frame_end);
          if(d_frame_start == d_frame_end) {
              d_state = GATHER_SAMPLES;
          }
          break;
      default:
          assert( 0 );
          break;
   };
  }

  consume_each(ii);  //
  return oo;	
}

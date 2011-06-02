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

using namespace std;

// public constructor that takes existing message queue
rfidbts_preamble_det_sptr
rfidbts_make_preamble_det(gr_msg_queue_sptr shared_q,
                          int samples_per_frame,
                          float detection_threshold)
{
  return rfidbts_preamble_det_sptr(new rfidbts_preamble_det(shared_q,
                                                            samples_per_frame,
                                                            detection_threshold));
}

rfidbts_preamble_det::rfidbts_preamble_det(gr_msg_queue_sptr shared_q,
                                           int samples_per_frame,
                                           float detection_threshold) : 
	gr_block("detector",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(0, 0, 0)),
    d_shared_q(shared_q),
    d_samples_per_frame(samples_per_frame),
    d_detection_threshold(detection_threshold),
    d_sample_count(0)
{
    vector<float>::iterator it;
    int len = 8;
    float half_sym[] = {1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0};
    float half_sym_neg[] = {-1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0};
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
    //reverse the sequence to get match filter
    reverse(d_rpreamble.begin(),d_rpreamble.end());

    d_correlator = new gri_fir_filter_with_buffer_ccf(d_rpreamble);
    d_samp_buffer = new gr_complex[d_samples_per_frame];
    d_correlation_buffer = new gr_complex[d_samples_per_frame];
}

rfidbts_preamble_det::~rfidbts_preamble_det() {
    delete d_correlator;
    delete d_samp_buffer;
    delete d_correlation_buffer;
}

int rfidbts_preamble_det::find_frame_start() {
    float best_so_far = 0.0;
    float mag_sqr;
    int index = -1;
    int ii;

    d_correlator->filterN(d_correlation_buffer,
                          d_samp_buffer,
                          d_samples_per_frame - d_rpreamble.size());
    for(ii = d_rpreamble.size(); ii < d_samples_per_frame - d_rpreamble.size(); ii++) {
        mag_sqr = d_correlation_buffer[ii].real() * d_correlation_buffer[ii].real() +
                  d_correlation_buffer[ii].imag() * d_correlation_buffer[ii].imag();
        if(mag_sqr > best_so_far) {
            best_so_far = mag_sqr;
            index = ii;
        }
    }

    if(best_so_far < d_detection_threshold) {
        index = -1;
    }

    cout << "Preamble at: " << index << endl;

    return index;
}

void rfidbts_preamble_det::send_frame_samples(int start_index) {
    gr_complex *frame_start;
    string message_frame;
    
    if(start_index == -1) {
        return;
    }
    //use the preamble to lock onto timing. . .16 repeating pattern + one extra frame because of group delay
    frame_start = d_samp_buffer + start_index - d_rpreamble.size() - (16 * 2 * 8 + 8)  + 1;
    message_frame = string((char*) frame_start, (d_samples_per_frame - start_index + d_rpreamble.size() + (16 * 2 * 8 + 8) - 1) * sizeof(gr_complex));
    d_shared_q->insert_tail(gr_make_message_from_string(message_frame));
    
    cout << "Preamble detected sent frame to decoder" << endl;
}

int rfidbts_preamble_det::general_work(int noutput_items,
				     gr_vector_int &ninput_items,
				     gr_vector_const_void_star &input_items,
				     gr_vector_void_star &output_items)
{
  int ii = 0;
  const gr_complex *in = (const gr_complex *) input_items[0];
  
  while(ii < ninput_items[0]) {
    if(d_sample_count < d_samples_per_frame) {
        d_samp_buffer[d_sample_count] = in[ii];
        ii++;
        d_sample_count++;
    }
    else {
        send_frame_samples( find_frame_start() );
        d_sample_count = 0;
    }
  }

  consume_each(noutput_items);  // Use all the inputs
  return 0;	
}

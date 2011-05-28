/* -*- c++ -*- */
/*
 * Copyright 2006 Free Software Foundation, Inc.
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

#ifndef INCLUDED_RFIDBTS_PREAMBLE_DET_H
#define	INCLUDED_RFIDBTS_PREAMBLE_DET_H

#include <gr_block.h>
#include <vector>
#include <gri_fir_filter_with_buffer_ccf.h>
#include <gr_msg_queue.h>
#include <gr_message.h>

class rfidbts_preamble_det;
typedef boost::shared_ptr<rfidbts_preamble_det> rfidbts_preamble_det_sptr;

rfidbts_preamble_det_sptr rfidbts_make_preamble_det(gr_msg_queue_sptr shared_q, 
                                                    int samples_per_frame,
                                                    float detection_threshold);

class rfidbts_preamble_det : public gr_block
{
private:
    gr_msg_queue_sptr d_shared_q;
    int d_samples_per_frame;
    float d_detection_threshold;

    int d_sample_count;
    gr_complex *d_samp_buffer;
    gr_complex *d_correlation_buffer;

    gri_fir_filter_with_buffer_ccf *d_correlator;
    
    std::vector<float> d_rpreamble;

    int find_frame_start();
    void send_frame_samples(int start_index);


    friend rfidbts_preamble_det_sptr rfidbts_make_preamble_det(gr_msg_queue_sptr shared_q,
                                                               int samples_per_frame,
                                                               float detection_threshold);

protected:
  rfidbts_preamble_det(gr_msg_queue_sptr shared_q,
                       int samples_per_frame,
                       float detection_threshold);

public:
  ~rfidbts_preamble_det();

  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif

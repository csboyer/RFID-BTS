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

rfidbts_preamble_det_sptr rfidbts_make_preamble_det(float detection_threshold,
                                                    int samples_in_frame,
                                                    int samples_out_frame);

class rfidbts_preamble_det : public gr_block
{
    enum State {GATHER_SAMPLES, SEARCH, OUTPUT_FRAME};
private:
    int d_samples_per_frame;
    int d_samples_per_outframe;
    int d_offset_from_preamble;
    float d_detection_threshold;
    int d_sample_count;

    State d_state;
    int d_frame_start;
    int d_frame_end;

    int d_num_phases;
    gr_complex *d_samp_buffer;
    gr_complex **d_phase_shifted;
    gr_complex *d_correlation_buffer;

    gri_fir_filter_with_buffer_ccf *d_correlator;
    gri_fir_filter_with_buffer_ccf *d_phase_shifter;
    std::vector<float> d_rpreamble;

    int find_frame_start();
    int send_frame_samples(int req_samples, gr_complex *out);


    friend rfidbts_preamble_det_sptr rfidbts_make_preamble_det(float detection_threshold,
                                                               int samples_in_frame,
                                                               int samples_out_frame);

protected:
  rfidbts_preamble_det(float detection_threshold,
                       int samples_in_frame,
                       int samples_out_frame);

public:
  ~rfidbts_preamble_det();

  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif

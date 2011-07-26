
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

#ifndef INCLUDED_RFIDBTS_ELG_TIMING_CC_H
#define	INCLUDED_RFIDBTS_ELG_TIMING_CC_H

#include <gr_block.h>
#include <gr_complex.h>
#include <gr_math.h>
#include <queue>
#include <gr_msg_queue.h>
#include <gr_message.h>
#include <rfidbts_controller.h>

class gri_mmse_fir_interpolator_cc;

class rfidbts_elg_timing_cc;
typedef boost::shared_ptr<rfidbts_elg_timing_cc> rfidbts_elg_timing_cc_sptr;

// public constructor
rfidbts_elg_timing_cc_sptr rfidbts_make_elg_timing_cc (float phase_offset, 
                                                       float samples_per_symbol, 
                                                       float dco_gain, 
                                                       float order_1_gain, 
                                                       float order_2_gain);

class rfidbts_elg_timing_cc : public gr_block
{
 public:
  ~rfidbts_elg_timing_cc ();
  void forecast(int noutput_items, gr_vector_int &ninput_items_required);
  int general_work (int noutput_items,
		            gr_vector_int &ninput_items,
		            gr_vector_const_void_star &input_items,
		            gr_vector_void_star &output_items);
  void set_verbose (bool verbose) { d_verbose = verbose; }

  void set_phase_offset(float p) { d_po = p; }
  
  void set_spb (float spb) {
      d_spb = spb;
      d_min_spb = spb*(1.0 - d_spb_relative_limit);
      d_max_spb = spb*(1.0 + d_spb_relative_limit);
      d_spb_mid = 0.5*(d_min_spb+d_max_spb);
  }

  void set_queue(gr_msg_queue_sptr q);


protected:
  rfidbts_elg_timing_cc (float phase_offset, 
                         float samples_per_symbol, 
                         float dco_gain, 
                         float order_1_gain, 
                         float order_2_gain);

 private:

  enum State {INIT, SYMBOL_TRACK, SAMPLE_DELAY};
  State d_state;

  int debug_count;
  float d_po;
  float d_po_init;
  float d_spb;
  float d_spb_init;
  float d_dco_gain;
  float d_order_1_gain;
  float d_order_2_gain;

  float d_integrator_1;
  float d_loopfilter;

  int d_symbol_counter;
  int d_delay_counter;

  int d_early_sample;
  float d_early_po;
  int d_late_sample;
  float d_late_po;

  float d_spb_relative_limit;
  float d_min_spb;
  float d_max_spb;
  float d_spb_mid;

  gri_mmse_fir_interpolator_cc 	*d_interp;
  bool			        d_verbose;

  gr_complex                    d_s_early;
  gr_complex                    d_s_0T;
  gr_complex                    d_s_late;

  gr_msg_queue_sptr d_queue;
  std::queue<rfidbts_controller::symbol_sync_task> d_task_queue;

  void reset_elg_values();
  void update_loopfilter(float timing_error);
  int update_dco();
  float gen_output_error(const gr_complex *in);
  int search_tag(int offset, int input_items);
  void fetch_and_process_task();

  friend rfidbts_elg_timing_cc_sptr rfidbts_make_elg_timing_cc (float phase_offset, 
                                                                float samples_per_symbol, 
                                                                float dco_gain, 
                                                                float order_1_gain, 
                                                                float order_2_gain);
};

#endif

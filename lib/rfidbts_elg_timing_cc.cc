
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
#include <gr_tag_info.h>
#include <rfidbts_elg_timing_cc.h>
#include <rfidbts_controller.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <cfloat>

#define GARDNER_DEBUG


#include <iostream>

extern rfidbts_controller_sptr rfid_mac;

using namespace std;

// Public constructor
static const int FUDGE = 16;

rfidbts_elg_timing_cc_sptr rfidbts_make_elg_timing_cc (float phase_offset, 
                                                       float samples_per_symbol, 
                                                       float dco_gain, 
                                                       float order_1_gain, 
                                                       float order_2_gain)
{
  return gnuradio::get_initial_sptr(new rfidbts_elg_timing_cc (phase_offset,
                                                               samples_per_symbol,
                                                               dco_gain,
                                                               order_1_gain,
                                                               order_2_gain));
}

rfidbts_elg_timing_cc::rfidbts_elg_timing_cc(float phase_offset, 
                                             float samples_per_symbol, 
                                             float dco_gain, 
                                             float order_1_gain, 
                                             float order_2_gain)
  : gr_block ("elg_timing_cc",
	          gr_make_io_signature (1, 1, sizeof (gr_complex)),
	          gr_make_io_signature (1, 1, sizeof (gr_complex))),
    d_po_init(phase_offset), 
    d_spb_init(samples_per_symbol),
    d_spb_relative_limit(0.05),
    d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("elg_timing_cc", "verbose", false)),
    d_s_late(0), d_s_0T(0), d_s_early(0),
    d_dco_gain(dco_gain),
    d_order_1_gain(order_1_gain),
    d_order_2_gain(order_2_gain),
    d_state(INIT)
{
  tag_propagation_policy_t p;
  p = TPP_DONT;
  set_tag_propagation_policy(p);

  set_spb(d_spb_init);
  set_relative_rate(1 / d_spb_init);
  set_history(4);

  reset_elg_values();
  debug_count = 0;

}

rfidbts_elg_timing_cc::~rfidbts_elg_timing_cc ()
{
  delete d_interp;
}

void rfidbts_elg_timing_cc::reset_elg_values() {
    d_po = d_po_init;
    d_spb = d_spb_init;
    d_early_po = d_po - 0.5;
    d_early_sample = (int) floor(d_early_po);
    d_early_po = d_early_po - d_early_sample;

    d_late_po = d_po + 0.5;
    d_late_sample = (int) floor(d_late_po);
    d_late_po = d_late_po - d_late_sample;
    d_integrator_1 = 0.0;
}

void rfidbts_elg_timing_cc::set_queue(gr_msg_queue_sptr q) {
    d_queue = q;
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
    d_early_po = d_po - 0.5;
    d_early_sample = (int) floor(d_early_po);
    d_early_po = d_early_po - d_early_sample;
    //calc late point 1/4 sample in future
    d_late_po = d_po + 0.5;
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

  //offset by one sample so it is possible to go back in time
  in++;
  while( ii < ni && oo < noutput_items) {
      switch(d_state) {
          case INIT:
              ii += search_tag(ii, ni);
              break;
          case SYMBOL_TRACK:
              if(d_symbol_counter > 0) {
                  //calc error and update loop filter
                  update_loopfilter(gen_output_error(in + ii));
                  //whole samples to advance by
                  samples_consumed = update_dco();
                  if(samples_consumed + ii > ni) {
                      d_delay_counter = samples_consumed + ii - ni;
                      ii = ni;
                      d_state = SAMPLE_DELAY;
                  }
                  else {
                      ii += samples_consumed;
                  }
                  //write output
                  out[oo] = d_s_0T;
                  oo++;
                  d_symbol_counter--;

                  if(d_symbol_counter == 0) {
                      goto work_end;
                  }
              }
              else {
                  fetch_and_process_task();
              }
              break;
          case SAMPLE_DELAY:
              if(d_delay_counter > 0) {
                  samples_consumed = min(ni - ii, d_delay_counter);
                  ii += samples_consumed;
                  d_delay_counter = d_delay_counter - samples_consumed;
              }
              else {
                  d_state = SYMBOL_TRACK;
              }
              break;
          default:
              assert(0);
      };
  }
work_end:
  assert(ii <= ninput_items[0] );
  consume_each(ii);
  return oo;
}

int rfidbts_elg_timing_cc::search_tag(int offset, int input_items) {
    vector<pmt::pmt_t> tags;
    rfidbts_controller::symbol_sync_task task;
    int count;

    get_tags_in_range(tags, 0,
                      nitems_read(0) + offset, nitems_read(0) + input_items,
                      pmt::pmt_string_to_symbol("rfid_burst"));
    //see if the tag marking the start of the stream exists, if so start there.
    if(tags.begin() != tags.end()) {
        sort(tags.begin(), tags.end(), gr_tags::nitems_compare);
        assert(pmt::pmt_is_true(gr_tags::get_value(tags[0])));
        fetch_and_process_task();
        cout << "ELG found tag: " << gr_tags::get_nitems(tags[0]) << endl;
        count = gr_tags::get_nitems(tags[0]) - nitems_read(0);
        //re init po and spb
        reset_elg_values();
    }
    else {
        count = input_items - offset;
        cout << "ELG tag not found: " << nitems_read(0) + offset << " To " << nitems_read(0) + input_items << endl;
    }

    return count;
}

void rfidbts_elg_timing_cc::fetch_and_process_task() {
    gr_message_sptr msg;
    int size;
    unsigned char *msg_buf;
    rfidbts_controller::symbol_sync_task task_buf[16];

    if(d_task_queue.empty()) {
        msg = d_queue->delete_head();
        msg_buf = msg->msg();
        memcpy(&size, msg_buf, sizeof(int));
        assert(size < 16);
        memcpy(task_buf, msg_buf + sizeof(int), sizeof(rfidbts_controller::symbol_sync_task) * size);
        //make queue
        for(int ii = 1; ii < size; ii++) {
            d_task_queue.push(task_buf[ii]);
        }
    }
    else {
        task_buf[0] = d_task_queue.front();
        d_task_queue.pop();
    }

    switch(task_buf[0].cmd) {
        case rfidbts_controller::SYM_TRACK_CMD:
            cout << "ELG received SYM track" << endl;
            d_symbol_counter = task_buf[0].output_symbol_len;
            if(task_buf[0].match_filter_offset > 0) {
                d_state = SAMPLE_DELAY;
                d_delay_counter = task_buf[0].match_filter_offset;
            }
            else {
                d_state = SYMBOL_TRACK;
            }
            break;
        case rfidbts_controller::SYM_DONE_CMD:
            cout << "ELG received end task" << endl;
            d_state = INIT;
            break;
        default:
            assert(0);
    };
}

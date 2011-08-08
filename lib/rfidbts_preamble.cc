/* -*- c++ -*- */
/*
 * Copyright 2007,2010 Free Software Foundation, Inc.
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

#include <rfidbts_preamble.h>
#include <rfidbts_controller.h>
#include <gr_io_signature.h>
#include <string.h>
#include <iostream>
#include <gr_message.h>
#include <gr_tag_info.h>

//#define PREAMBLE_DEBUG

extern rfidbts_controller_sptr rfid_mac;

using namespace std;
////////////////////////////////////////
rfidbts_preamble_gate_sptr rfidbts_make_preamble_gate ()
{
  return gnuradio::get_initial_sptr(new rfidbts_preamble_gate ());
}

rfidbts_preamble_gate::rfidbts_preamble_gate()
    : gr_block("preamble_gate",
               gr_make_io_signature(1, 1, sizeof(gr_complex)),
               gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_state(PG_TAG_SEARCH)
{
    tag_propagation_policy_t p;
    p = TPP_DONT;
    set_tag_propagation_policy(p);
}

rfidbts_preamble_gate::~rfidbts_preamble_gate() {
}

int
rfidbts_preamble_gate::general_work (int noutput_items,
                                     gr_vector_int &ninput_items,
	                                 gr_vector_const_void_star &input_items,
		                             gr_vector_void_star &output_items) {
    int ii;
    int ni;
    int oo;
    int m;
    gr_complex *in;
    gr_complex *out;
    vector<pmt::pmt_t> tags;
    rfidbts_controller::preamble_gate_task task;

    ii = 0;
    ni = ninput_items[0];
    oo = 0;
    in = (gr_complex*) input_items[0];
    out = (gr_complex*) output_items[0];
    while(ii < ni && oo < noutput_items) {
        switch(d_state) {
            case PG_TAG_SEARCH:
                //search for tag. If found, start preamble srch. If not keep searching for tag.
                tags.clear();
                get_tags_in_range(tags, 0, 
                                  nitems_read(0) + (uint64_t) ii, 
                                  nitems_read(0) + (uint64_t) ni,
                                  pmt::pmt_string_to_symbol("rfid_burst"));
                if(tags.begin() != tags.end()) {
                    sort(tags.begin(), tags.end(), gr_tags::nitems_compare);
                    assert(pmt::pmt_is_true(gr_tags::get_value(tags[0])));
                    ii = gr_tags::get_nitems(tags[0]) - (uint64_t) nitems_read(0);
                    rfid_mac->preamble_gate_callback(task);
                    //call to controller block
                    d_counter = task.len;
                    assert(d_counter > 0);
                    ii += INIT_OFFSET;
                    //check to see if overflow
                    if(ii > ni) {
                        d_timer = ni - ii;
                        d_state = PG_TIMER;
                        ii = ni;
                    } 
                    else {
                        d_state = PG_UNGATE;
                    }
                }
                else {
                    ii = ni;
                }
                break;
            case PG_TIMER:
                if(d_timer > 0) {
                    m = min(ni - ii, d_timer);
                    ii += m;
                    d_timer = d_timer - m;
                }
                else {
                    d_state = PG_UNGATE;
                }
            case PG_UNGATE:
                if(d_counter > 0) {
                    m = min(ni - ii, min(d_counter, noutput_items - oo));
                    memcpy(out + oo, in + ii, sizeof(gr_complex) * m);
                    ii += m;
                    oo += m;
                    d_counter = d_counter - m;
                }
                else {
                    d_state = PG_TAG_SEARCH;
                    //bail fast! force a context switch!
                    goto work_exit;
                }
                break;
            default:
                assert(0);
        };
    }
work_exit:
    consume_each(ii);
    return oo;
}
////////////////////////////////////////////////////////
rfidbts_preamble_srch_sptr rfidbts_make_preamble_srch ()
{
  return gnuradio::get_initial_sptr(new rfidbts_preamble_srch ());
}

rfidbts_preamble_srch::rfidbts_preamble_srch()
    : gr_sync_block("preamble_srch",
               gr_make_io_signature2(2, 2, sizeof(char), sizeof(float)),
               gr_make_io_signature(0, 0, 0)),
    d_state(PS_NEXT_FRAME),
    d_counter(0)
{
    tag_propagation_policy_t p;
    p = TPP_DONT;
    set_tag_propagation_policy(p);
}

rfidbts_preamble_srch::~rfidbts_preamble_srch() {
}

int
rfidbts_preamble_srch::work (int noutput_items,
	                         gr_vector_const_void_star &input_items,
		                     gr_vector_void_star &output_items) {
    int ii;
    int m;
    int offset;
    char *in;
    rfidbts_controller::preamble_srch_task task;

    ii = 0;
    in = (char*) input_items[0];
    while(ii < noutput_items) {
        switch(d_state) {
            case PS_NEXT_FRAME:
                if(d_counter > 0) {
                    m = min(d_counter, noutput_items - ii);
                    ii += m;
                    d_counter = d_counter - m;
                }
                else {
                    //call back to controller
                    rfid_mac->preamble_srch_callback(task);
                    d_counter = task.len;
                    d_frame_len = task.len;
                    d_state = PS_STROBE_SEARCH;
                }
                break;
            case PS_STROBE_SEARCH:
                assert(d_counter > 0);
#ifdef PREAMBLE_DEBUG
                cout << "srch counter" << d_counter << endl;
#endif
                m = min(d_counter, noutput_items - ii);
                if(find_strobe(in + ii, in + ii + m, &offset)) {
                    //queue up commands
                    task.len = offset + d_frame_len - d_counter;
                    task.srch_success = true;
                    rfid_mac->preamble_align_setup(task);
                    //adjust state vars
                    d_state = PS_NEXT_FRAME;
                    d_counter = d_counter - m;
                    ii += m;
                    goto work_exit;
                }
                d_counter = d_counter - m;
                ii += m;
                if((d_counter-50) <= 0) {
                    //send failure message
                    task.srch_success = false;
                    rfid_mac->preamble_align_setup(task);
                    d_state = PS_NEXT_FRAME;
                }
                break;
            default:
                assert(0);
        };
    }
work_exit:
    return ii;
}

bool rfidbts_preamble_srch::find_strobe(char *start, char *end, int *offset) {
    int ii = 0;
    while(start != end && *start == 0) {
        start++;
        ii++;
    }
    *offset = ii;
    return start != end;
}
////////////////////////////////////////////////////
rfidbts_preamble_align_sptr rfidbts_make_preamble_align ()
{
  return gnuradio::get_initial_sptr(new rfidbts_preamble_align ());
}

rfidbts_preamble_align::rfidbts_preamble_align()
    : gr_block("preamble_align",
               gr_make_io_signature(1, 1, sizeof(gr_complex)),
               gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_state(PA_TAG_SEARCH),
    d_task_queue()
{
    tag_propagation_policy_t p;
    p = TPP_DONT;
    set_tag_propagation_policy(p);
}

rfidbts_preamble_align::~rfidbts_preamble_align() {
}

void rfidbts_preamble_align::set_queue(gr_msg_queue_sptr q) {
    d_queue = q;
}

int
rfidbts_preamble_align::general_work (int noutput_items,
                                      gr_vector_int &ninput_items,
			                          gr_vector_const_void_star &input_items,
			                          gr_vector_void_star &output_items) {
    int ii;
    int ni;
    int oo;
    int m;
    gr_complex *in;
    gr_complex *out;
    vector<pmt::pmt_t> tags;
    stringstream str;
    pmt::pmt_t k;
    pmt::pmt_t v;
    pmt::pmt_t i;

    ii = 0;
    ni = ninput_items[0];
    oo = 0;
    in = (gr_complex*) input_items[0];
    out = (gr_complex*) output_items[0];

    while(ii < ni && oo < noutput_items) {
        switch(d_state) {
            case PA_TAG_SEARCH:
                get_tags_in_range(tags, 0, 
                                  nitems_read(0) + (uint64_t) ii, 
                                  nitems_read(0) + (uint64_t) ni,
                                  pmt::pmt_string_to_symbol("rfid_burst"));
                if(tags.begin() != tags.end()) {
#ifdef PREAMBLE_DEBUG
                    cout << "Preamble align found tag" << endl;
#endif
                    sort(tags.begin(), tags.end(), gr_tags::nitems_compare);
                    assert(pmt::pmt_is_true(gr_tags::get_value(tags[0])));
                    ii = gr_tags::get_nitems(tags[0]) - (uint64_t) nitems_read(0);
                    //call to controller block queue
                    fetch_and_process_task();
                }
                else {
                    ii = ni;
                }
                break;
            case PA_ALIGN:
                if(d_counter > 0) {
                    m = min(d_counter, ni - ii);
                    d_counter = d_counter - m;
                    ii += m;
                }
                else {
                    fetch_and_process_task();
                }
                break;
            case PA_UNGATE:
                if(d_counter > 0) {
                    //copy and update counters; bail if finished, process queue next time around
                    m = min(ni - ii, min(d_counter, noutput_items - oo));
                    memcpy(out + oo, in + ii, m * sizeof(gr_complex));
                    d_counter = d_counter - m;
                    ii += m;
                    oo += m;
                    if(d_counter == 0) {
                        goto work_exit;
                    }
                }
                else {
                    fetch_and_process_task();
                }
                break;
            case PA_TAG:
                //create tag
                str << name() << unique_id();
                k = pmt::pmt_string_to_symbol("rfid_burst");
                v = pmt::PMT_T;
                i = pmt::pmt_string_to_symbol(str.str());
                add_item_tag(0, nitems_written(0) + oo, k, v, i);
#ifdef PREAMBLE_DEBUG
                cout << "Align tagging sample: " << nitems_written(0) + oo << endl;
#endif
                //copy the output
                out[oo] = in[ii];
                ii++;
                oo++;
                //get the next task
                fetch_and_process_task();
                break;
            default:
                assert(0);
        };
    }
work_exit:
    consume_each(ii);
    return oo;
}

void rfidbts_preamble_align::fetch_and_process_task() {
    gr_message_sptr msg;
    int size;
    rfidbts_controller::preamble_align_task buf[16];
    
    if(d_task_queue.empty()) {
#ifdef PREAMBLE_DEBUG
        cout << "Align task queue is empty, getting more tasks" << endl;
#endif
        msg = d_queue->delete_head();
#ifdef PREAMBLE_DEBUG
        cout << "Align task queue finished" << endl;
#endif
        memcpy(&size, msg->msg(), sizeof(int));
        assert(size < 16);
        memcpy(buf, msg->msg() + sizeof(int), sizeof(rfidbts_controller::preamble_align_task) * size);
        //make queue
        for(int ii = 1; ii < size; ii++) {
            d_task_queue.push(buf[ii]);
        }
    }
    else {
#ifdef PREAMBLE_DEBUG
        cout << "Align task queue not empty." << endl;
#endif
        buf[0] = d_task_queue.front();
        d_task_queue.pop();
    }

    switch(buf[0].cmd) {
        case rfidbts_controller::PA_ALIGN_CMD:
#ifdef PREAMBLE_DEBUG
            cout << "PA_ALIGN task: " << buf[0].len << endl;
#endif
            d_state = PA_ALIGN;
            d_counter = buf[0].len;
            break;
        case rfidbts_controller::PA_TAG_CMD:
#ifdef PREAMBLE_DEBUG
            cout << "PA_TAG task " << endl;
#endif
            d_state = PA_TAG;
            break;
        case rfidbts_controller::PA_UNGATE_CMD:
#ifdef PREAMBLE_DEBUG
            cout << "PA_UNGATE task: " << buf[0].len << endl;
#endif
            d_state = PA_UNGATE;
            d_counter = buf[0].len;
            break;
        case rfidbts_controller::PA_DONE_CMD:
#ifdef PREAMBLE_DEBUG
            cout << "PA_DONE task " << endl;
#endif
            d_state = PA_TAG_SEARCH;
            break;
        default:
            assert(0);
    };
}
//////////////////////////////////////////////////


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

#include <rfidbts_receive_gate.h>
#include <cmath>
#include <gr_io_signature.h>
#include <stdio.h>
#include <iostream>

using namespace std;

// public constructor that takes existing message queue
rfidbts_receive_gate_sptr
rfidbts_make_receive_gate (float threshold,
                           int off_max)
{
  return rfidbts_receive_gate_sptr(new rfidbts_receive_gate(threshold,
                                                            off_max));
}

rfidbts_receive_gate::rfidbts_receive_gate(float threshold,
                                           int off_max)
	: gr_block("receiver_gate",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_threshold(threshold),
    d_off_max(off_max),
    d_counter(10000),
    d_state(ST_BOOTUP)
{
  tag_propagation_policy_t p;

  p = TPP_DONT;
  set_tag_propagation_policy(p);
}


void rfidbts_receive_gate::set_gate_queue(gr_msg_queue_sptr q) {
    d_cmd_queue = q;
}

int rfidbts_receive_gate::general_work(
                     int noutput_items,
				     gr_vector_int &ninput_items,
				     gr_vector_const_void_star &input_items,
				     gr_vector_void_star &output_items)
{
    int ii = 0;
    int oo = 0;
    int nii = (int) ninput_items[0];
    int m;
    float mag_sqrd;
    const gr_complex *in = (const gr_complex*) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];
    rfidbts_controller::rx_burst_task task;

    while(ii < nii && oo < noutput_items) {
        switch(d_state) {
            case ST_BOOTUP:
                if(d_counter == 0) {
                    d_state = ST_TXOFF;
                }
                else {
                    d_counter = d_counter - 1;
                    ii++;
                }
                break;
            case ST_TXOFF:
                if( abs(in[ii]) > d_threshold ) {
                    d_state = ST_TXON_MUTE;
                }
                else {
                   ii++;
                }
                break;
            case ST_TXON_MUTE:
                if( abs(in[ii]) < d_threshold ) {
                    d_counter = 0;
                    d_state = ST_TXOFF_MUTE;
                }
                else {
                    ii++;
                }
                break;
            case ST_TXOFF_MUTE:
                if(d_counter > d_off_max) {
                    d_state = ST_TXOFF;
                }
                else if( abs(in[ii]) > d_threshold  ) {
                    d_state = ST_COMMAND;
                }
                else {
                    d_counter++;
                    ii++;
                }
                break;
            case ST_COMMAND:
                if(!d_cmd_queue->empty_p()) {
                    //copy out command
                    process_cmd_queue(&task);
                    decode_task(task, oo);
                }
                else {
                    goto work_exit;
                }
                break;
            case ST_WAIT:
                if(d_counter > 0) {
                    m = min(d_counter, nii - ii);
                    d_counter = d_counter - m;
                    ii += m;
                }
                else {
                    d_state = ST_COMMAND;
                }
                break;
            case ST_UNMUTE:
                if(d_counter > 0) {
                    m = min(d_counter, min(noutput_items - oo, nii - ii));
                    memcpy(out + oo, in + ii, m * sizeof(gr_complex));
                    d_counter = d_counter - m;
                    oo += m;
                    ii += m;
                }
                else {
                    d_state = ST_COMMAND;
                }
                break;
            default:
                assert(0);
                break;
        };
    }

work_exit:
    consume_each(ii);
    return oo;
}

void rfidbts_receive_gate::add_tag(int offset) {
    stringstream str;

    str << name() << unique_id();
    pmt::pmt_t k = pmt::pmt_string_to_symbol("rfid_burst");
    pmt::pmt_t v = pmt::PMT_T;
    pmt::pmt_t i = pmt::pmt_string_to_symbol(str.str());
    add_item_tag(0, nitems_written(0) + offset, k, v, i);
}

void rfidbts_receive_gate::process_cmd_queue(rfidbts_controller::rx_burst_task *task) {
    gr_message_sptr msg;

    msg = d_cmd_queue->delete_head();
    memcpy(task, msg->msg(), sizeof(rfidbts_controller::rx_burst_task));

}

void rfidbts_receive_gate::decode_task(rfidbts_controller::rx_burst_task &task, int oo) {
    switch(task.cmd) {
        case rfidbts_controller::RXB_GATE:
            d_counter = task.len;
            d_state = ST_WAIT;
            cout << "Gate received RXB_GATE" << endl;
            break;
        case rfidbts_controller::RXB_UNGATE:
            add_tag(oo);
            d_counter = task.len;
            d_state = ST_UNMUTE;
            cout << "Gate received RXB_UNGATE" << endl;
            break;
        case rfidbts_controller::RXB_DONE:
            d_state = ST_TXOFF;
            cout << "Gate received RXB_DONE" << endl;
            break;
        default:
            assert(0);
    };
}


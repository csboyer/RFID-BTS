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

#include <rfidbts_pick.h>
#include <gr_io_signature.h>
#include <string.h>
#include <iostream>
#include <gr_message.h>

#define PICK_PEAK_DEBUG
#define PEAK_COUNT_DEBUG
#define MUX_DEBUG

using namespace std;

rfidbts_pick_peak_sptr rfidbts_make_pick_peak ()
{
  return gnuradio::get_initial_sptr(new rfidbts_pick_peak ());
}

rfidbts_pick_peak::rfidbts_pick_peak ()
  : gr_sync_block ("pick_peak",
		   gr_make_io_signature (1, 256, sizeof(peak_count_pair)),
		   gr_make_io_signature (0, 0, 0))
{
}

int
rfidbts_pick_peak::work (int noutput_items,
			             gr_vector_const_void_star &input_items,
			             gr_vector_void_star &output_items) {
    int connections;
    int ii;
    int best_index;
    int best_correlator;
    float best_so_far;
    peak_count_pair *in;
    gr_message_sptr msg;
    rfidbts_mux_slice_dice::mux_ctrl_msg *ctrl_msg;

    assert(noutput_items > 0);
    connections = input_items.size();
    best_so_far = -(float)INFINITY;

    for(ii = 0; ii < connections; ii++) {
        in = (peak_count_pair*) input_items[ii];
        if(in->peak > best_so_far) {
            best_so_far = in->peak;
            best_correlator = ii;
            best_index = in->count;
        }
    }

    msg = gr_make_message(0, 
                          sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg), 
                          2, 
                          2 * sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg));
    ctrl_msg = (rfidbts_mux_slice_dice::mux_ctrl_msg*) msg->msg();

    if(best_so_far > 0.0) {
#ifdef PICK_PEAK_DEBUG
        cout << "Best peak: " << best_so_far << " Found at index: " <<
                 best_index << " Correlator: " << best_correlator << endl;
#endif
        //want to offset so the preamble can be used for loop filter settle time
        generate_mux_commands(false, best_correlator, best_index + 1 - (12 + 7 + 1) * 8, ctrl_msg);
        ctrl_msg++;
        generate_mux_commands(true, best_correlator, 500, ctrl_msg);
        ctrl_msg++;
        for(ii = 0; ii < connections; ii++) {
            if(ii != best_correlator) {
                //add delete functionality next
            }
        }
        assert(d_queue);
        d_queue->handle(msg);
    }
    else {
    }
    
    return 1; 
}

void rfidbts_pick_peak::generate_mux_commands(bool copy, char in_sel, int len, void* buf) {
    rfidbts_mux_slice_dice::mux_ctrl_msg m;

    m.len = len;
    m.in_sel = in_sel;
    m.copy = copy;

    memcpy(buf, &m, sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg));
}
///////////////////

rfidbts_peak_count_stream_sptr rfidbts_make_peak_count_stream (int frame_sample_len, int preamble_thresh)
{
  return gnuradio::get_initial_sptr(new rfidbts_peak_count_stream (frame_sample_len, preamble_thresh));
}

rfidbts_peak_count_stream::rfidbts_peak_count_stream (int frame_sample_len, int preamble_thresh)
  : gr_block ("pick_peak",
		   gr_make_io_signature2 (2, 2, sizeof(char), sizeof(float)),
		   gr_make_io_signature (1, 1, sizeof(rfidbts_pick_peak::peak_count_pair))),
    d_state(SEARCH_ACTIVE),
    d_count(0),
    d_frame_sample_len(frame_sample_len),
    d_preamble_thresh(preamble_thresh)
{
}

int
rfidbts_peak_count_stream::general_work (int noutput_items,
                                         gr_vector_int &ninput_items,
			                             gr_vector_const_void_star &input_items,
			                             gr_vector_void_star &output_items) {
    rfidbts_pick_peak::peak_count_pair buf;
    char *in_strobe = (char*) input_items[0];
    float *peak = (float*) input_items[1];
    rfidbts_pick_peak::peak_count_pair *out = (rfidbts_pick_peak::peak_count_pair*) output_items[0];
    int ii;
    int oo;
    int min_in;

    assert(noutput_items > 0);
    oo = 0;
    min_in = min(ninput_items[0], ninput_items[1]);
    for(ii = 0; ii < min_in; ii++) {
        switch(d_state) {
            case SEARCH_ACTIVE:
                //look for a detected peak, note the location in the sample stream. Send to pick max block
                if(in_strobe[ii]) {
#ifdef PEAK_COUNT_DEBUG
                    cout << "Found frame at: " << d_count << endl;
#endif
                    buf.peak = peak[ii];
                    buf.count = d_count;
                    memcpy(out, &buf, sizeof(rfidbts_pick_peak::peak_count_pair));
                    oo++;
                    d_state = WAIT_NEW_FRAME;
                }
                else if(d_count >= d_preamble_thresh) {
#ifdef PEAK_COUNT_DEBUG
                    cout << "Couldn't find frame, giving up: " << d_count << endl;
#endif
                    //couldn't find a peak within the frame. Give up and signal to main block
                    buf.peak = -1.0;
                    memcpy(out, &buf, sizeof(rfidbts_pick_peak::peak_count_pair));
                    oo++;
                    d_state = WAIT_NEW_FRAME;
                }
                d_count++;
                break;
            case WAIT_NEW_FRAME:
                //ignore any peaks until the frame rollover
                if(d_count >= d_frame_sample_len) {
                    d_count = 0;
                    d_state = SEARCH_ACTIVE;
#ifdef PEAK_COUNT_DEBUG
                    cout << "Roll over occured" << endl;
#endif
                }
                else {
                    d_count++;
                }
                break;
            default:
                assert(0);
        };
    }
    
    consume_each(ii);
    return oo; 
}

/////////MUX
rfidbts_mux_slice_dice_sptr rfidbts_make_mux_slice_dice(int itemsize) {
  return gnuradio::get_initial_sptr(new rfidbts_mux_slice_dice (itemsize));
}

rfidbts_mux_slice_dice::rfidbts_mux_slice_dice(int itemsize) :
    gr_block("mux_slice_dice",
             gr_make_io_signature(1, 256, itemsize),
             gr_make_io_signature(1, 1, itemsize)),
    d_itemsize(itemsize),
    d_state(WAIT)
{
}

int rfidbts_mux_slice_dice::general_work(int noutput_items,
                                         gr_vector_int &ninput_items,
                                         gr_vector_const_void_star &input_items,
                                         gr_vector_void_star &output_items) {
    vector<int> ii;
    gr_message_sptr current_msg;
    bool loop;
    int oo;
    int smallest;
    char sel;
    char *in;
    char *out = (char*) output_items[0];

    loop = true;
    oo = 0;
    ii.resize(input_items.size(), 0);
    while(loop) {
        switch(d_state) {
            case WAIT:
                if(d_queue->empty_p()) {
                    //nothing in the queue to process bail
                    loop = false;
                }
                else {
#ifdef MUX_DEBUG
                    cout << "Slice and dice received new command" << endl;
#endif
                    //new command in the queue; pop off
                    d_current_msg = d_queue->delete_head();
                    assert(d_current_msg->length() % sizeof(mux_ctrl_msg) == 0);
                    d_cmds_to_process = (int) d_current_msg->arg2() - 1;
                    d_current_cmd = (mux_ctrl_msg*) d_current_msg->msg();
                    //set the state for the next command
                    d_state = (d_current_cmd->copy) ? COPY : DELETE;
#ifdef MUX_DEBUG
                    string s = (d_current_cmd->copy)  ? string("COPY") : string("DELETE");
                    cout << "Going to " << s << " state" << endl;
#endif
                }
                break;
            case COPY:
                //check to see if any more items can be copied; not enough output buffer or not enough input buffer
                if(oo == noutput_items || ii[d_current_cmd->in_sel] == ninput_items[d_current_cmd->in_sel]) {
                    loop = false;
                }
                else {
                    //figure out how many items we can process
                    smallest = min(noutput_items - oo, 
                                   min(ninput_items[d_current_cmd->in_sel], d_current_cmd->len));
                    //find correct offset in input buffer
                    in = (char*) input_items[d_current_cmd->in_sel] + ii[d_current_cmd->in_sel] * d_itemsize;
                    //copy and update different fields
                    memcpy(out + oo * d_itemsize, in, smallest * d_itemsize);
#ifdef MUX_DEBUG
                    cout << "S_D copying " << smallest  << " items" << endl;
#endif
                    oo += smallest;
                    ii[d_current_cmd->in_sel] += smallest;
                    d_current_cmd->len = d_current_cmd->len - smallest;
                    //see if we are to process next cmd in message
                    pop_next_cmd();
                }
                break;
            case DELETE:
                //check to see if any more items can be copied; not enough input buffer
                if(ii[d_current_cmd->in_sel] == ninput_items[d_current_cmd->in_sel]) {
                    loop = false;
                }
                else {
                    //figure out how many items we can process
                    smallest = min(ninput_items[d_current_cmd->in_sel], d_current_cmd->len);
#ifdef MUX_DEBUG
                    cout << "S_D deleting " << smallest  << " items" << endl;
#endif
                    //update different fields
                    ii[d_current_cmd->in_sel] += smallest;
                    d_current_cmd->len = d_current_cmd->len - smallest;
                    //see if we are to process next cmd in message
                    pop_next_cmd();
                }
                break;
            default:
                assert(0);
        };
    }

    for(int iter = 0; iter < ii.size(); iter++) {
        consume(iter, ii[iter]);
    }
    return oo;
}

void rfidbts_mux_slice_dice::pop_next_cmd() {
    assert(d_current_cmd->len >= 0);

    if(d_current_cmd->len > 0) {
        //do nothing
    }
    else if(d_cmds_to_process > 0) {
        d_current_cmd++;
        d_state = (d_current_cmd->copy) ? COPY : DELETE;
        d_cmds_to_process--;
#ifdef MUX_DEBUG
                    string s = (d_current_cmd->copy)  ? string("COPY") : string("DELETE");
                    cout << "Going to " << s << " state" << endl;
#endif
    }
    else {
        d_state = WAIT;
    }
}


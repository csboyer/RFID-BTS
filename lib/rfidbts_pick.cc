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
#include <rfidbts_controller.h>
#include <gr_io_signature.h>
#include <string.h>
#include <iostream>
#include <gr_message.h>
#include <gr_tag_info.h>

extern rfidbts_controller_sptr rfid_mac;

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
    int ii;
    int best_index;
    int best_correlator;
    int connections;
    float best_so_far;
    peak_count_pair *in;
    gr_message_sptr msg;
    rfidbts_controller::preamble_search_task task;

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


    if(best_so_far > 0.0) {
#ifdef PICK_PEAK_DEBUG
        cout << "Best peak: " << best_so_far << " Found at index: " <<
                 best_index << " Correlator: " << best_correlator << endl;
#endif
        rfid_mac->preamble_search(true, task);
        //want to offset so the preamble can be used for loop filter settle time
        msg = generate_message();
        generate_mux_commands(rfidbts_mux_slice_dice::MUX_DELETE, 
                              best_correlator, best_index + 1 - (12 + 7 + 1) * 8, msg);
        d_queue->handle(msg);
        //copy out start of frame!
        msg = generate_message();
        generate_mux_commands(rfidbts_mux_slice_dice::MUX_COPY, best_correlator, 2 * 8, msg);
        d_queue->handle(msg);

        msg = generate_message();
        generate_mux_commands(rfidbts_mux_slice_dice::MUX_TAG, best_correlator, 0, msg);
        d_queue->handle(msg);

        msg = generate_message();
        generate_mux_commands(rfidbts_mux_slice_dice::MUX_COPY, 
                              best_correlator, 480, msg);
        d_queue->handle(msg);
    }
    else {
        rfid_mac->preamble_search(false, task);
    }

    flush_all_buffers(connections);

    return 1; 
}

void rfidbts_pick_peak::flush_all_buffers(int connections) {
    gr_message_sptr msg;

    assert(d_queue);
    for(char ii = 0; ii < connections; ii++) {
        msg = generate_message();
        generate_mux_commands(rfidbts_mux_slice_dice::MUX_FLUSH,
                              ii, -1, msg);
        d_queue->handle(msg);
    }
}

gr_message_sptr rfidbts_pick_peak::generate_message() {
    return gr_make_message(0,
                           sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg),
                           1,
                           sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg));
}

void rfidbts_pick_peak::generate_mux_commands(rfidbts_mux_slice_dice::mux_ctrl_cmd cmd, char in_sel, int len, gr_message_sptr msg) {
    rfidbts_mux_slice_dice::mux_ctrl_msg m;

    m.len = len;
    m.in_sel = in_sel;
    m.cmd = cmd;

    memcpy(msg->msg(), &m, sizeof(rfidbts_mux_slice_dice::mux_ctrl_msg));
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
    vector<pmt::pmt_t> tags;

    assert(noutput_items > 0);
    oo = 0;
    ii = 0;
    min_in = min(ninput_items[0], ninput_items[1]);
    while(ii < min_in) {
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
                ii++;
                d_count++;
                break;
            case WAIT_NEW_FRAME:
                get_tags_in_range(tags, 0, nitems_read(0) + ii, 
                                  nitems_read(0) + min_in, pmt::pmt_string_to_symbol("rfidbts_burst"));
                sort(tags.begin(), tags.end(), gr_tags::nitems_compare);
                if(tags.begin() != tags.end()) {
                    assert(pmt::pmt_is_true(gr_tags::get_value(tags[0])));
                    ii = gr_tags::get_nitems(tags[0]) - nitems_read(0);
                    d_count = 0;
                    d_state = SEARCH_ACTIVE;
                }
                else {
                    ii = min_in;
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
    tag_propagation_policy_t p;
    //do not want to propagate any of the tags....make new ones!
    p = TPP_DONT;
    set_tag_propagation_policy(p);
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
                    current_msg = d_queue->delete_head();
                    assert(current_msg->length() == sizeof(mux_ctrl_msg));
                    memcpy(&d_current_cmd, current_msg->msg(), sizeof(mux_ctrl_msg));
                    //set the state for the next command
                    set_state();
                }
                break;
            case COPY:
                //check to see if any more items can be copied; not enough output buffer or not enough input buffer
                if(oo == noutput_items || ii[d_current_cmd.in_sel] == ninput_items[d_current_cmd.in_sel]) {
                    loop = false;
                }
                else {
                    //figure out how many items we can process
                    smallest = min(noutput_items - oo, 
                                   min(ninput_items[d_current_cmd.in_sel], d_current_cmd.len));
                    //find correct offset in input buffer
                    in = (char*) input_items[d_current_cmd.in_sel] + ii[d_current_cmd.in_sel] * d_itemsize;
                    //copy and update different fields
                    memcpy(out + oo * d_itemsize, in, smallest * d_itemsize);
#ifdef MUX_DEBUG
                    cout << "S_D copying " << smallest  << " items" << endl;
#endif
                    oo += smallest;
                    ii[d_current_cmd.in_sel] += smallest;
                    d_current_cmd.len = d_current_cmd.len - smallest;
                    //see if we are to process next cmd in message
                    pop_next_cmd();
                }
                break;
            case TAG_COPY:
                //check to see if any more items can be copied; not enough output buffer or not enough input buffer
                if(oo == noutput_items || ii[d_current_cmd.in_sel] == ninput_items[d_current_cmd.in_sel]) {
                    loop = false;
                }
                else {
                    in = (char*) input_items[d_current_cmd.in_sel] + ii[d_current_cmd.in_sel] * d_itemsize;
                    memcpy(out + oo * d_itemsize, in, d_itemsize);
                    tag_symbol_boundry(oo);                   
                    oo++;
                    ii[d_current_cmd.in_sel]++;
                    d_state = WAIT;
                }
                break;
            case DELETE:
                //check to see if any more items can be copied; not enough input buffer
                if(ii[d_current_cmd.in_sel] == ninput_items[d_current_cmd.in_sel]) {
                    loop = false;
                }
                else {
                    //figure out how many items we can process
                    smallest = min(ninput_items[d_current_cmd.in_sel], d_current_cmd.len);
#ifdef MUX_DEBUG
                    cout << "S_D deleting " << smallest  << " items" << endl;
#endif
                    //update different fields
                    ii[d_current_cmd.in_sel] += smallest;
                    d_current_cmd.len = d_current_cmd.len - smallest;
                    //see if we are to process next cmd in message
                    pop_next_cmd();
                }
                break;
            case DELETE_FLUSH:
                if(ii[d_current_cmd.in_sel] == ninput_items[d_current_cmd.in_sel]) {
                    loop = false;
                }
                else {
                    ii[d_current_cmd.in_sel] += search_for_tag(ii[d_current_cmd.in_sel], ninput_items[d_current_cmd.in_sel]);
                }
                break;
            default:
                assert(0);
                break;
        };
    }

    for(int iter = 0; iter < ii.size(); iter++) {
        consume(iter, ii[iter]);
    }
    return oo;
}

int rfidbts_mux_slice_dice::search_for_tag(int offset, int input_items) {
    vector<pmt::pmt_t> tags;
    
    get_tags_in_range(tags, 0, 
                      nitems_read(d_current_cmd.in_sel) + offset, 
                      nitems_read(d_current_cmd.in_sel) + input_items, 
                      pmt::pmt_string_to_symbol("rfidbts_burst"));
    if(tags.begin() != tags.end()) {
        sort(tags.begin(), tags.end(), gr_tags::nitems_compare);
        assert(pmt::pmt_is_true(gr_tags::get_value(tags[0])));
        d_state = WAIT;
        return gr_tags::get_nitems(tags[0]) - 
               (nitems_read(d_current_cmd.in_sel) + offset);
    }
    else {
        return input_items - offset;
    }
}

void rfidbts_mux_slice_dice::tag_symbol_boundry(int offset) {
    stringstream str;

    str << name() << unique_id();
    pmt::pmt_t k = pmt::pmt_string_to_symbol("rfidbts_burst");
    pmt::pmt_t v = pmt::PMT_T;
    pmt::pmt_t i = pmt::pmt_string_to_symbol(str.str());
    add_item_tag(0, nitems_written(0) + offset, k, v, i);
}

void rfidbts_mux_slice_dice::set_state() {
    switch(d_current_cmd.cmd) {
        case MUX_DELETE:
            d_state = DELETE;
            break;
        case MUX_COPY:
            d_state = COPY;
            break;
        case MUX_FLUSH:
            d_state = DELETE_FLUSH;
            break;
        case MUX_TAG:
            d_state = TAG_COPY; 
            break;
        default:
            assert(0);
    };
}

void rfidbts_mux_slice_dice::pop_next_cmd() {
    assert(d_current_cmd.len >= 0);

    if(d_current_cmd.len == 0) {
        d_state = WAIT;
    }
}


/////////////////////////////////////////////////////

rfidbts_packetizer_sptr rfidbts_make_packetizer() {
  return gnuradio::get_initial_sptr(new rfidbts_packetizer());
}

rfidbts_packetizer::rfidbts_packetizer() :
    gr_sync_block("packetizer",
             gr_make_io_signature(1, 1, sizeof(char)),
             gr_make_io_signature(0, 0, 0))
{
}

int rfidbts_packetizer::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items) {
    int t;
    int ii = 0;
    char *in;
    rfidbts_controller::bit_decode_task task;

    in = (char*) input_items[0];

    while(ii < noutput_items) {
        switch(d_state) {
            case IDLE:
                //call mac layer
                rfid_mac->bit_decode(task);
                assert(task.valid);
                d_bit_offset = task.bit_offset;
                d_packet_len = task.output_bit_len;
                d_packet.resize(task.output_bit_len);
                d_state = OFFSET;
                break;
            case OFFSET:
                t = min(noutput_items - ii, d_bit_offset);
                ii += t;
                d_bit_offset = d_bit_offset - t;

                if(d_bit_offset == 0) {
                    d_state = GET_FRAME;
                }
                break;
            case GET_FRAME:
                //copy
                t = min(noutput_items - ii, d_packet_len);
                copy(in + ii, in + ii + t, d_packet.end() - d_packet_len);
                //update pointers
                ii += t;
                d_packet_len = d_packet_len - t;
                //copy
                if(d_packet_len == 0) {
                    d_state = IDLE;
                    rfid_mac->decoded_message(d_packet);
                    //call mac layer
                }
                break;
            default:
                assert(0);
        };
    }
    return ii;
}


/* -*- c++ -*- */
/*
 * Copyright 2005,2010,2011 Free Software Foundation, Inc.
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

#include <iostream>

#include <rfidbts_pie_encoder.h>
#include <gr_io_signature.h>
#include <gr_complex.h>

#include <cstdio>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdexcept>
#include <string.h>
#include <assert.h>

//#define PIE_DEBUG

using namespace std;
// public constructor that returns a shared_ptr

rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder(
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1) {
    return rfidbts_pie_encoder_sptr(
          new rfidbts_pie_encoder(
              samples_per_delimiter,
              samples_per_tari,
              samples_per_pw,
              samples_per_trcal,
              samples_per_data1));
}

rfidbts_pie_encoder::rfidbts_pie_encoder (
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1) : gr_block(
            "pie_encoder",
		    gr_make_io_signature(0, 0, 0),
		    gr_make_io_signature(1, 1, sizeof(gr_complex))),
        d_state(OFF_IDLE),
        d_sample_1(1.0,1.0),
        d_sample_0(0.0,0.0)
{
    int samples_per_rtcal = samples_per_data1 + samples_per_tari;
    vector<gr_complex>::iterator it;
    
    //setup d_data0
    d_data0.resize(samples_per_tari);
    fill(
            d_data0.begin(), 
            d_data0.end() - samples_per_pw, 
            d_sample_1);
    fill(
            d_data0.end() - samples_per_pw, 
            d_data0.end(),
            d_sample_0);
    //setup d_data1
    d_data1.resize(samples_per_data1);
    fill(
            d_data1.begin(), 
            d_data1.end() - samples_per_pw, 
            d_sample_1);
    fill(
            d_data1.end() - samples_per_pw, 
            d_data1.end(),
            d_sample_0);
    //setup preamble
    d_preamble.resize(
            samples_per_delimiter + 
            d_data0.size() + 
            samples_per_trcal + 
            samples_per_rtcal);

    it = d_preamble.begin();
    //delimiter
    fill(
            it,
            it + samples_per_delimiter,
            d_sample_0);
    it += samples_per_delimiter;
    //data0
    copy(
            d_data0.begin(),
            d_data0.end(),
            it);
    it += d_data0.size();
    //rtcal
    fill(
            it,
            it + samples_per_rtcal - samples_per_pw,
            d_sample_1);
    assert(samples_per_rtcal > samples_per_pw);
    it += samples_per_rtcal - samples_per_pw;
    fill(
            it,
            it + samples_per_pw,
            d_sample_0);
    it += samples_per_pw;
    //trcal
    fill(
            it,
            it + samples_per_trcal - samples_per_pw,
            d_sample_1);
    it += samples_per_trcal - samples_per_pw;
    fill(
            it,
            it + samples_per_pw,
            d_sample_0);
    it += samples_per_pw;
    //setup framesync
    d_framesync.resize(
            samples_per_delimiter +
            d_data0.size() +
            samples_per_rtcal);
    it = d_framesync.begin();
    //delimiter
    fill(
            it,
            it + samples_per_delimiter,
            d_sample_0);
    it += samples_per_delimiter;
    //data0
    copy(
            d_data0.begin(),
            d_data0.end(),
            it);
    it += d_data0.size();
    //rtcal
    fill(
            it,
            it + samples_per_rtcal - samples_per_pw,
            d_sample_1);
    it += samples_per_rtcal - samples_per_pw;
    fill(
            it,
            it + samples_per_pw,
            d_sample_0);
    it += samples_per_pw;
}

void rfidbts_pie_encoder::set_encoder_queue(gr_msg_queue_sptr q) {
    d_cmd_queue = q;
}

rfidbts_pie_encoder::~rfidbts_pie_encoder()
{
}

void rfidbts_pie_encoder::bit_to_pie(
        const vector<unsigned char> *b, 
        vector<gr_complex> &p) {
    vector<unsigned char>::const_iterator it;
    vector<gr_complex>::iterator filler;
    int sample_cnt = 0;
    
    for(it = b->begin();it != b->end();it++) {
        assert(*it == 0 || *it == 1);
        sample_cnt += *it == 0 ? d_data0.size() : d_data1.size();
    }
    p.assign(sample_cnt, d_sample_0);
    filler = p.begin();
    for(it = b->begin();it != b->end();it++) {
        if(*it == 0) {
            copy(
                    d_data0.begin(),
                    d_data0.end(),
                    filler);
            filler += d_data0.size();
        }
        else {
            copy(
                    d_data1.begin(),
                    d_data1.end(),
                    filler);
            filler += d_data1.size();
        }
    }
    assert(filler == p.end());
} 


int rfidbts_pie_encoder::general_work(
        int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
{
    int nn = 0;
    int ni = noutput_items;
    int m;
    gr_complex *output = (gr_complex*) output_items[0];
    rfidbts_controller::tx_burst_task task;
    ni = min(512, noutput_items);
    
    while(nn < ni) {
        switch(d_state) {
            case OFF_IDLE:
                if(!d_cmd_queue->empty_p()) {
                    get_task(&task);
                    decode_task(&task);
                }
                else {
                    fill(output + nn,
                         output + ni,
                         d_sample_0);
                    nn = ni;
                }
                break;
            case ON_IDLE:
                if(!d_cmd_queue->empty_p()) {
                    get_task(&task);
                    decode_task(&task);
                }
                else {
                    fill(output + nn,
                         output + ni,
                         d_sample_1);
                    nn = ni;
                }
                break;
            case ON_TIMED:
                if(d_counter > 0) {
                    m = min(d_counter, ni - nn);
                    fill(output + nn,
                         output + m + nn,
                         d_sample_1);
                    nn += m;
                    d_counter = d_counter - m;
                }
                else {
                    d_state = GET_TASK;
                }
                break;
            case SEND_SAMPLES:
                if(d_counter < d_outgoing_frame.size()) {
                    m = min((int) d_outgoing_frame.size() - d_counter, ni - nn);
                    copy(d_outgoing_frame.begin() + d_counter,
                         d_outgoing_frame.begin() + d_counter + m,
                         output + nn);
                    nn += m;
                    d_counter += m;
                }
                else {
                    //cout << "PIE Blk: PIE samples written: " << nitems_written(0) + nn << endl;
                    d_state = GET_TASK;
                }
                break;
            case GET_TASK:
                if(!d_cmd_queue->empty_p()) {
                    get_task(&task);
                    decode_task(&task);
                }
                else {
                    goto work_exit;
                }
                break;
            default:
                assert(0);
        };
    }

work_exit:

    return nn;
}

void rfidbts_pie_encoder::print_sent_samples() {
    cout << "Outside blk: PIE samples written: " << nitems_written(0) << endl;
}

void rfidbts_pie_encoder::get_task(rfidbts_controller::tx_burst_task *task) {
    gr_message_sptr msg;

    msg = d_cmd_queue->delete_head();
    memcpy(task, msg->msg(), msg->length());
}

void rfidbts_pie_encoder::decode_task(rfidbts_controller::tx_burst_task *task) {

    switch(task->cmd) {
        case rfidbts_controller::TURN_ON_TIMED:
#ifdef PIE_DEBUG
            cout << "pie_encoder received TURN_ON_TIMED task" << endl;
#endif
            d_counter = task->len;
            d_state = ON_TIMED;
            break;
        case rfidbts_controller::TURN_ON:
#ifdef PIE_DEBUG
            cout << "pie_encoder received TURN_ON task" << endl;
#endif
            d_state = ON_IDLE;
            break;
        case rfidbts_controller::PREAMBLE:
#ifdef PIE_DEBUG
            cout << "pie_encoder received PREAMBLE task" << endl;
#endif
            d_outgoing_frame.assign(d_preamble.begin(), d_preamble.end());
            d_state = SEND_SAMPLES;
            d_counter = 0;
            break;
        case rfidbts_controller::FRAME_SYNCH:
#ifdef PIE_DEBUG
            cout << "pie_encoder received FRAME_SYNCH task" << endl;
#endif
            d_outgoing_frame.resize(d_framesync.size());
            d_outgoing_frame.assign(d_framesync.begin(), d_framesync.end());
            d_state = SEND_SAMPLES;
            d_counter = 0;
            break;
        case rfidbts_controller::DATA_BITS:
#ifdef PIE_DEBUG
            cout << "pie_encoder received DATA_BITS task" << endl;
#endif
            bit_to_pie(task->data, d_outgoing_frame);
            delete task->data;
            d_state = SEND_SAMPLES;
            d_counter = 0;
            break;
        case rfidbts_controller::TURN_OFF:
#ifdef PIE_DEBUG
            cout << "pie_encoder received TURN_OFF task" << endl;
#endif
            d_state = OFF_IDLE;
            break;
        default:
            assert(0);
    };
}


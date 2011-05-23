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

#define D_PIE
#ifdef D_PIE
#include <iostream>
#endif

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

using namespace std;
// public constructor that returns a shared_ptr

rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder(
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1,
        int samples_per_cw,
        int samples_per_wait) {
    return rfidbts_pie_encoder_sptr(
          new rfidbts_pie_encoder(
              samples_per_delimiter,
              samples_per_tari,
              samples_per_pw,
              samples_per_trcal,
              samples_per_data1,
              samples_per_cw,
              samples_per_wait));
}

rfidbts_pie_encoder::rfidbts_pie_encoder (
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1,
        int samples_per_cw,
        int samples_per_wait) : gr_block(
            "pie_encoder",
		    gr_make_io_signature(0, 0, 0),
		    gr_make_io_signature(1, 1, sizeof(gr_complex))),
        d_current_state(idle),
        d_sample_1(1.0,1.0),
        d_sample_0(0.0,0.0),
        d_samples_per_cw(samples_per_cw),
        d_samples_per_wait(samples_per_wait),
        d_frameq(5)
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

rfidbts_pie_encoder::~rfidbts_pie_encoder()
{
}

bool rfidbts_pie_encoder::make_frame(
        const vector<gr_complex> &header, 
        const vector<unsigned char> &data) {
    vector<gr_complex> pie_data;
    vector<gr_complex> final_frame;
#ifdef D_PIE
    cout << "Making frame " << endl;
#endif
    bit_to_pie(data, pie_data);
    final_frame.resize(header.size() + pie_data.size());

    copy(
            header.begin(),
            header.end(),
            final_frame.begin());
    copy(
            pie_data.begin(),
            pie_data.end(),
            final_frame.begin() + header.size());

    if(!d_frameq.full_p()) {
        d_frameq.insert_tail(gr_make_message_from_string(
                   string((char*) &final_frame[0], final_frame.size() * sizeof(gr_complex))));
        return true;
    }
    else {
        return false;
    }
}

bool rfidbts_pie_encoder::snd_frame_preamble(const vector<unsigned char> &data) {
    return make_frame(d_preamble, data);
}

bool rfidbts_pie_encoder::snd_frame_framesync(const vector<unsigned char> &data) {
    return make_frame(d_framesync, data);
}

void rfidbts_pie_encoder::bit_to_pie(
        const vector<unsigned char> &b, 
        vector<gr_complex> &p) {
    vector<unsigned char>::const_iterator it;
    vector<gr_complex>::iterator filler;
    int sample_cnt = 0;
    
    for(it = b.begin();it != b.end();it++) {
        assert(*it == 0 || *it == 1);
        sample_cnt += *it == 0 ? d_data0.size() : d_data1.size();
    }
    
    p.resize(sample_cnt);
    filler = p.begin();
    for(it = b.begin();it != b.end();it++) {
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

void rfidbts_pie_encoder::ready_msgframe(gr_message_sptr frame_msg) {
    assert(frame_msg != 0);
    assert(frame_msg->length() % sizeof(gr_complex) == 0);
    
    gr_complex *frame = (gr_complex*) frame_msg->msg();

    d_outgoing_frame.resize(frame_msg->length() / sizeof(gr_complex));
    copy(
            frame, 
            frame + d_outgoing_frame.size(),
            d_outgoing_frame.begin());

    d_num_sent_cw = 0;
    d_num_sent_frame = 0;
    d_num_sent_wait = 0;
}

int rfidbts_pie_encoder::general_work(
        int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
{
    int nn = 0;
    gr_complex *output = (gr_complex*) output_items[0];
    
    if(noutput_items == 0) {
        return nn;
    }

    switch(d_current_state) {
        case idle:
            if(!d_frameq.empty_p()) {
                ready_msgframe(d_frameq.delete_head_nowait());
                nn++;
                fill(
                        output,
                        output + nn,
                        d_sample_0);
                d_current_state = send_cw;
#ifdef D_PIE
                cout << "State idle to send_cw" << endl;
#endif
            }
            else {
                nn += noutput_items;
                fill(
                        output,
                        output + nn,
                        d_sample_0);
            }
            break;
        case send_cw:
            if(d_num_sent_cw < d_samples_per_cw) {
                nn = min(
                        d_samples_per_cw - d_num_sent_cw,
                        noutput_items);
                assert(nn >= 0);
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_num_sent_cw += nn;
            }
            else {
#ifdef D_PIE
                cout << "Send_cw to send frame" << endl;
#endif
                nn++;
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_current_state = send_frame;
            }
            break;
        case send_frame:
            if(d_num_sent_frame < d_outgoing_frame.size()) {
                nn = min((int) d_outgoing_frame.size() - d_num_sent_frame, noutput_items);
                assert(nn >= 0);

                copy(
                        d_outgoing_frame.begin() + d_num_sent_frame,
                        d_outgoing_frame.begin() + d_num_sent_frame + nn,
                        output);
                d_num_sent_frame += nn;
            }
            else {
#ifdef D_PIE
                cout << "frame to wait for frame" << endl;
#endif
                nn++;
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_current_state = wait_for_frame;
            }
            break;
        case wait_for_frame:
            if(!d_frameq.empty_p()) {
                ready_msgframe(d_frameq.delete_head_nowait());
#ifdef D_PIE
                cout << "wait for frame to frame" << endl;
#endif
                nn++;
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_current_state = send_frame;
            }
            else if(d_num_sent_wait < d_samples_per_wait) {
                nn = min(
                        d_samples_per_wait - d_num_sent_wait,
                        noutput_items);
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_num_sent_wait += nn;
            }
            else {
#ifdef D_PIE
                cout << "wait for frame to idle" << endl;
#endif
                nn++;
                fill(
                        output,
                        output + nn,
                        d_sample_1);
                d_current_state = idle;
            }
            break;
        default:
            assert(false);
            break;
    };
    return nn;
}



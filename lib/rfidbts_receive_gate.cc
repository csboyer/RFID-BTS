
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

#include "rfidbts_receive_gate.h"
#include <cmath>
#include <gr_io_signature.h>
#include <stdio.h>
#include <iostream>

using namespace std;

// public constructor that takes existing message queue
rfidbts_receive_gate_sptr
rfidbts_make_receive_gate (float threshold,
                           int pw_preamble,
                           int off_max,
                           int mute_buffer,
                           int tag_response)
{
  return rfidbts_receive_gate_sptr(new rfidbts_receive_gate(threshold,
                                                            pw_preamble,
                                                            off_max,
                                                            mute_buffer,
                                                            tag_response));
}

rfidbts_receive_gate::rfidbts_receive_gate(float threshold,
                                           int pw_preamble,
                                           int off_max,
                                           int mute_buffer,
                                           int tag_response)
	: gr_block("receiver_gate",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_threshold(threshold),
    d_pw_preamble(pw_preamble),
    d_off_max(off_max),
    d_mute_buffer(mute_buffer),
    d_tag_response(tag_response),
    d_bootup_count(0),
    d_bootup_time(300),
    d_state(ST_BOOTUP)
{
}

int rfidbts_receive_gate::general_work(
                     int noutput_items,
				     gr_vector_int &ninput_items,
				     gr_vector_const_void_star &input_items,
				     gr_vector_void_star &output_items)
{
    int ii;
    int oo = 0;
    int nii = (int) ninput_items[0];
    float mag_sqrd;
    const gr_complex *in = (const gr_complex*) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];

    for(ii = 0; ii < nii; ii++) {
        switch(d_state) {
            case ST_BOOTUP:
                if(d_bootup_count > d_bootup_time) {
                    d_state = ST_TXOFF;
                }
                else {
                    d_bootup_count++;
                }
                break;
            case ST_TXOFF:
                if( abs(in[ii]) > (d_threshold ) ) {
                    d_state = ST_TXON_MUTE;
                    d_pw_count = 0;
                }
                break;
            case ST_TXON_MUTE:
                if( abs(in[ii]) < (d_threshold ) ) {
                    d_off_count = 0;
                    d_pw_count++;
                    d_state = ST_TXOFF_MUTE;
                }
                break;
            case ST_TXOFF_MUTE:
                if(d_off_count > d_off_max) {
                    d_state = ST_TXOFF;
                }
                else if( abs(in[ii]) > d_threshold  && d_pw_count >= d_pw_preamble) {
                    d_unmute_count = 0;
                    d_state = ST_UNMUTE;
                }
                else if( abs(in[ii]) > d_threshold  ) {
                    d_state = ST_TXON_MUTE;
                }
                else {
                    d_off_count++;
                }
                break;
            case ST_UNMUTE:
                if(d_unmute_count < d_mute_buffer) {
                    d_unmute_count++;
                }
                else if(d_unmute_count > d_tag_response) {
                    d_state = ST_TXON_MUTE;
                    d_pw_count = 0;
                }
                else if(d_unmute_count == d_mute_buffer) {
                    stringstream str;
                    str << name() << unique_id();
                    pmt::pmt_t k = pmt::pmt_string_to_symbol("burst");
                    pmt::pmt_t v = pmt::PMT_T;
                    pmt::pmt_t i = pmt::pmt_string_to_symbol(str.str());
                    add_item_tag(0, nitems_written(0) + ii, k, v, i);
                    out[oo] = in[ii];
                    oo++;
                    d_unmute_count++;
                }
                else {
                    out[oo] = in[ii];
                    oo++;
                    d_unmute_count++;
                }
                break;
            default:
                assert(0);
                break;
        };
    }


    consume_each(ii);
    return oo;
}


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

// public constructor that takes existing message queue
rfidbts_receive_gate_sptr
rfidbts_make_receive_gate(int delimiter_samps,
                          int rx_samps,
                          int preamble_samps,
                          int wait_samps)
{
  return rfidbts_receive_gate_sptr(new rfidbts_receive_gate(delimiter_samps,
                                                            rx_samps,
                                                            preamble_samps,
                                                            wait_samps));
}

rfidbts_receive_gate::rfidbts_receive_gate(int delimiter_samps,
                                           int rx_samps,
                                           int preamble_samps,
                                           int wait_samps)
	: gr_block("receiver_gate",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_delimiter_samps(delimiter_samps),
    d_rx_samps(rx_samps),
    d_preamble_samps(preamble_samps),
    d_wait_samps(wait_samps),
    d_state(ST_TXOFF)
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
        case ST_TXOFF: //idle state
            if( abs(in[ii]) > 0.01 ) {
                d_state = ST_TXON_MUTE;
            }
            break;
        case ST_TXON_MUTE: //reader is charging the tag
            if( abs(in[ii]) < 0.015 ) {
                d_state = ST_DELIMITER;
                d_samp_cnt = 0;
            }
            break;
        case ST_DELIMITER: //detected potential delimiter
            if(d_samp_cnt < d_delimiter_samps) {
                d_samp_cnt++;
            }
            else if(abs(in[ii]) > 0.015) {
                d_state = ST_PREAMBLE_WAIT;
                d_samp_cnt = 0;
            }
            else {
                d_state = ST_TXOFF;
            }
            break;
        case ST_PREAMBLE_WAIT: //found the delimter, wait the entire length of preamble
            if(d_samp_cnt < d_preamble_samps) {
                d_samp_cnt++;
            }
            else {
                d_state = ST_RXON;
                d_samp_cnt = 0;
            }
            break;
        case ST_RXON: //potentially receiving tag backscatter
            if(d_samp_cnt < d_rx_samps) {
                d_samp_cnt++;
                out[oo] = in[ii];
                oo++;
            }
            else {
                d_state = ST_POSTRX;
                d_samp_cnt = 0;
            }
            break;
        case ST_POSTRX: //may be more backscatter or another downlink frame
            if(abs(in[ii]) < 0.015) {
                d_state = ST_DELIMITER;
                d_samp_cnt = 0;
            }
            else if(d_samp_cnt < d_wait_samps) {
                d_samp_cnt++;
                out[oo] = in[ii];
                oo++;
            }
            else {
                d_state = ST_TXON_MUTE;
            }
            break;
        default:
            assert(0);
            return -1;
    };
    }

    consume_each(ii);
    return oo;
}

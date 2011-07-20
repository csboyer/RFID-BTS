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
               gr_make_io_signature(1, 1, sizeof(gr_complex)))
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
    return 0;
}
////////////////////////////////////////////////////////
rfidbts_preamble_srch_sptr rfidbts_make_preamble_srch ()
{
  return gnuradio::get_initial_sptr(new rfidbts_preamble_srch ());
}

rfidbts_preamble_srch::rfidbts_preamble_srch()
    : gr_sync_block("preamble_srch",
               gr_make_io_signature2(2, 2, sizeof(char), sizeof(float)),
               gr_make_io_signature(-1, -1, 0))
{
    tag_propagation_policy_t p;
    p = TPP_DONT;
    set_tag_propagation_policy(p);
}

rfidbts_preamble_srch::~rfidbts_preamble_srch() {
}

void rfidbts_preamble_srch::set_queue(gr_msg_queue_sptr q) {
    d_queue = q;
}

int
rfidbts_preamble_srch::work (int noutput_items,
	                         gr_vector_const_void_star &input_items,
		                     gr_vector_void_star &output_items) {
    return 0;
}
////////////////////////////////////////////////////
rfidbts_preamble_align_sptr rfidbts_make_preamble_align ()
{
  return gnuradio::get_initial_sptr(new rfidbts_preamble_align ());
}

rfidbts_preamble_align::rfidbts_preamble_align()
    : gr_block("preamble_align",
               gr_make_io_signature(1, 1, sizeof(gr_complex)),
               gr_make_io_signature(1, 1, sizeof(gr_complex)))
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
    return 0;
}
//////////////////////////////////////////////////

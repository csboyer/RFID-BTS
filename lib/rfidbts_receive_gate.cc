
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
#include <gr_io_signature.h>
#include <stdio.h>
#include <list>


// public constructor that takes existing message queue
rfidbts_receive_gate_sptr
rfidbts_make_receive_gate(int preamble_samps, int wait_samps)
{
  return rfidbts_receive_gate_sptr(new rfidbts_receive_gate(preamble_samps, wait_samps));
}

rfidbts_receive_gate::rfidbts_receive_gate(int preamble_samps,
                                           int wait_samps)
	: gr_block("receiver_gate",
	         gr_make_io_signature(1, 1, sizeof(gr_complex)),
	         gr_make_io_signature(1, 1, sizeof(gr_complex))),
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
    consume_each(0);
}

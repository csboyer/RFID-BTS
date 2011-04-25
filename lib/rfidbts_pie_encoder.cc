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

using namespace std;
// public constructor that returns a shared_ptr

rfidbts_pie_encoder_sptr
rfidbts_make_pie_encoder(int msgq_limit)
{
  return rfidbts_pie_encoder_sptr(new rfidbts_pie_encoder(msgq_limit));
}

// public constructor that takes existing message queue
rfidbts_pie_encoder_sptr
rfidbts_make_pie_encoder(gr_msg_queue_sptr msgq)
{
  return rfidbts_pie_encoder_sptr(new rfidbts_pie_encoder(msgq));
}

rfidbts_pie_encoder::rfidbts_pie_encoder (int msgq_limit)
  : gr_block("pie_encoder",
		  gr_make_io_signature(0, 0, 0),
		  gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_msgq(gr_make_msg_queue(msgq_limit)),
    d_eof(false)
{
}

rfidbts_pie_encoder::rfidbts_pie_encoder (gr_msg_queue_sptr msgq)
  : gr_block("pie_encoder",
		  gr_make_io_signature(0, 0, 0),
		  gr_make_io_signature(1, 1, sizeof(gr_complex))),
    d_msgq(msgq), 
    d_eof(false)
{
}

rfidbts_pie_encoder::~rfidbts_pie_encoder()
{
}

void rfidbts_pie_encoder::bit_to_pie(gr_message_sptr command) 
{
	gr_complex sample_0 = gr_complex(0.0,0.0);
  	gr_complex sample_1 = gr_complex(1.0,0.0);
	unsigned char *bit_buffer;
	bit_buffer = command->msg(); 
	int rt_cal = 3;
	int tr_cal = 7.5;

	// encode preamble or frame sync
	if (bit_buffer[0] == '4') {  // preamble
		d_pie_symbols.push_back(sample_0);
		d_pie_symbols.push_back(sample_1);
		d_pie_symbols.push_back(sample_0);
		for (int i = 0; i < rt_cal * 2 - 1; i++) {
			d_pie_symbols.push_back(sample_1);
		}
		d_pie_symbols.push_back(sample_0);
		for (int i = 0; i < tr_cal * 2 - 1; i++) {
			d_pie_symbols.push_back(sample_1);
		}
		d_pie_symbols.push_back(sample_0);
	} else if (bit_buffer[0] == '5') {
		d_pie_symbols.push_back(sample_0);
		d_pie_symbols.push_back(sample_1);
		d_pie_symbols.push_back(sample_0);
		for (int i = 0; i < rt_cal * 2 - 1; i++) {
      d_pie_symbols.push_back(sample_1);
		}
		d_pie_symbols.push_back(sample_0);
	} else {
		throw std::runtime_error("Bad header code");
	}

	// encode the rest of the message

	for (int i = 1; i < command->length(); i++) {
		if (bit_buffer[i] == '0') {
			d_pie_symbols.push_back(sample_1);
			d_pie_symbols.push_back(sample_0);
		} else if (bit_buffer[i] == '1') {
			d_pie_symbols.push_back(sample_1);
			d_pie_symbols.push_back(sample_1);
			d_pie_symbols.push_back(sample_1);
			d_pie_symbols.push_back(sample_0);
		} else {
			throw std::runtime_error("input msg is not a char sequence of 1 or 0 bits");
		}
	}
			
} 

int
rfidbts_pie_encoder::general_work(int noutput_items,
      gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  char *out = (char *) output_items[0];
  int nn = 0;
  //setup constants for memcpy to use
  gr_complex sample_0 = gr_complex(0.0,0.0);
  gr_complex sample_1 = gr_complex(1.0,0.0);
  
  if (d_eof) {
    return -1;
  }
  //output as many as samples as needed
  while (nn < noutput_items){
    //if samples still available to send, add to output buffer
    if (!d_pie_symbols.empty()){
      gr_complex pie_sample = d_pie_symbols.front();
      d_pie_symbols.pop_front();
      memcpy (out, &pie_sample, sizeof(gr_complex));
      nn++;
      out += sizeof(gr_complex);
    } 
    //no more samples to send, check queue for a new message
    else if (!d_msgq->empty_p()){
      gr_message_sptr msg = d_msgq->delete_head();
      //check to see if the message is end of file, if so set flag and return the samples so far
      if (msg->type() == 1){
        d_eof = true;
        break;
      }
      //not end of message, convert bits to pie samples. Return before processing next
      bit_to_pie(msg);
      //break;
    }
    //nothing in the queue, just run at full power
    else {
      memcpy (out, &sample_1, sizeof(gr_complex));
      out += sizeof(gr_complex);
      nn++;
    }
  }

  return nn;
}


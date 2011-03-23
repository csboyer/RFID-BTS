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


// public constructor that returns a shared_ptr

rfidbts_pie_encoder_sptr
rfidbts_make_pie_encoder(int msgq_limit)
{
  return gnuradio::get_initial_sptr(new rfidbts_pie_encoder(msgq_limit));
}

// public constructor that takes existing message queue
rfidbts_pie_encoder_sptr
rfidbts_make_pie_encoder(gr_msg_queue_sptr msgq)
{
  return gnuradio::get_initial_sptr(new rfidbts_pie_encoder(msgq));
}

rfidbts_pie_encoder::rfidbts_pie_encoder (int msgq_limit)
  : gr_sync_block("pie_encoder",
		  gr_make_io_signature(0, 0, 0),
		  gr_make_io_signature(1, 1, sizeof(gr_complex))),
      d_msgq(gr_make_msg_queue(msgq_limit)),
      d_msg_offset(0), 
      d_counted_samples(0), 
      d_eof(false)
{
}

rfidbts_pie_encoder::rfidbts_pie_encoder (gr_msg_queue_sptr msgq)
  : gr_sync_block("pie_encoder",
		  gr_make_io_signature(0, 0, 0),
		  gr_make_io_signature(1, 1, sizeof(gr_complex))),
      d_msgq(msgq), 
      d_msg_offset(0),
      d_counted_samples(0), 
      d_eof(false)
{
}

rfidbts_pie_encoder::~rfidbts_pie_encoder()
{
}

void rfidbts_pie_encoder::bit_to_pie(gr_message &command) 
{
	gr_complex sample_0 = gr_complex(0.0,0.0);
  	gr_complex sample_1 = gr_complex(1.0,0.0);
	unsigned char *bit_buffer;
	bit_buffer = command.msg(); 
	int rt_cal = 3;
	int tr_cal = 8;

	// encode preamble or frame sync
	if (bit_buffer[0] == (unsigned char) 0xE) {  // preamble
		m_pie_symbols.push_back(sample_0);
		m_pie_symbols.push_back(sample_1);
		m_pie_symbols.push_back(sample_0);
		for (int i = 0; i < rt_cal * 2 - 1; i++) {
			m_pie_symbols.push_back(sample_1);
		}
		m_pie_symbols.push_back(sample_0);
		for (int i = 0; i < tr_cal * 2 - 1; i++) {
			m_pie_symbols.push_back(sample_1);
		}
		m_pie_symbols.push_back(sample_0);
	} else if (bit_buffer[0] == (unsigned char) 0xF) {
		m_pie_symbols.push_back(sample_0);
		m_pie_symbols.push_back(sample_1);
		m_pie_symbols.push_back(sample_0);
		for (int i = 0; i < rt_cal * 2 - 1; i++) {
			m_pie_symbols.push_back(sample_1);
		}
		m_pie_symbols.push_back(sample_0);
	} else {
		throw std::runtime_error("input msg is not a char sequence of 1 or 0 bits");
	}

	// encode the rest of the message

	for (int i = 1; i < command.length(); i++) {
		if (bit_buffer[i] == 0) {
			m_pie_symbols.push_back(sample_1);
			m_pie_symbols.push_back(sample_0);
		} else if (bit_buffer[i] == 1) {
			m_pie_symbols.push_back(sample_1);
			m_pie_symbols.push_back(sample_1);
			m_pie_symbols.push_back(sample_1);
			m_pie_symbols.push_back(sample_0);
		} else {
			throw std::runtime_error("input msg is not a char sequence of 1 or 0 bits");
		}
	}
			
} 

int
rfidbts_pie_encoder::work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  char *out = (char *) output_items[0];
  int nn = 0;
  //setup constants for memcpy to use
  gr_complex sample_0 = gr_complex(0.0,0.0);
  gr_complex sample_1 = gr_complex(1.0,0.0);

  while (nn < noutput_items){
    if (d_msg && d_msg_offset != d_msg->length()){
      //
      // Consume whatever we can from the current message
      //
      assert(d_msg_offset < d_msg->length());
      //
      // Generate a zero bit symbol
      //
      if (d_msg->msg()[d_msg_offset] == 0)
      {
        assert(d_counted_samples < 2);
        if (d_counted_samples == 0)
        {
          memcpy (out, &sample_1, sizeof(gr_complex));
          nn++;
          d_counted_samples++;
          out += sizeof(gr_complex);
          d_msg_offset += 1;
        }
        else
        {
          memcpy (out, &sample_0, sizeof(gr_complex));
          nn++;
          d_counted_samples = 0;
          out += sizeof(gr_complex);
          d_msg_offset += 1;
        }
      }
      //
      // Generate a one bit symbol
      //
      else if (d_msg->msg()[d_msg_offset] == 1)
      {
        assert(d_counted_samples < 3);
        if (d_counted_samples < 2)
        {
          memcpy (out, &sample_1, sizeof(gr_complex));
          nn++;
          d_counted_samples++;
          out += sizeof(gr_complex);
          d_msg_offset += 1;
        }
        else
        {
          memcpy (out, &sample_0, sizeof(gr_complex));
          nn++;
          d_counted_samples = 0;
          out += sizeof(gr_complex);
          d_msg_offset += 1;
        }
      }
      else
      {
        throw std::runtime_error("input msg is not a char sequence of 1 or 0 bits");
      }
      
    }
    else if (d_msg && d_msg_offset == d_msg->length()) {
      if (d_msg->type() == 1)	           // type == 1 sets EOF
	      d_eof = true;
	    d_msg.reset();
    }
    else {
      //
      // No current message
      //
      if (d_msgq->empty_p() && nn >= 0){    // return ones
        while ( nn < noutput_items)
        {
          memcpy (out, &sample_1, sizeof(gr_complex));
          nn++;
        }
        //go to end of method for clean up
	      break;
      }

      if (d_eof)
	      return -1;

      d_msg = d_msgq->delete_head();	   // block, waiting for a message
      d_msg_offset = 0;
    }
  }

  return nn;
}


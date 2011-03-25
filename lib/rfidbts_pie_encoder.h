/* -*- c++ -*- */
/*
 * Copyright 2005,2011 Free Software Foundation, Inc.
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

#ifndef INCLUDED_RFIDBTS_PIE_ENCODER_H
#define INCLUDED_RFIDBTS_PIE_ENCODER_H

#include <gr_block.h>
#include <gr_message.h>
#include <gr_msg_queue.h>


class rfidbts_pie_encoder;
typedef boost::shared_ptr<rfidbts_pie_encoder> rfidbts_pie_encoder_sptr;

rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (int msgq_limit=1);
rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (gr_msg_queue_sptr msgq);

/*!
 * \brief Turn received messages into a stream
 * \ingroup source_blk
 */
class rfidbts_pie_encoder : public gr_block
{
 private:

  gr_msg_queue_sptr	d_msgq;

  bool			d_eof;
  std::list<gr_complex>	d_pie_symbols;

  friend rfidbts_pie_encoder_sptr
  rfidbts_make_pie_encoder(int msgq_limit);
  friend rfidbts_pie_encoder_sptr
  rfidbts_make_pie_encoder(gr_msg_queue_sptr msgq);
  void bit_to_pie(gr_message_sptr command);
 protected:
  rfidbts_pie_encoder (int msgq_limit);
  rfidbts_pie_encoder (gr_msg_queue_sptr msgq);

 public:
  ~rfidbts_pie_encoder();

  gr_msg_queue_sptr	msgq() const { return d_msgq; }

  int general_work (int noutput_items,
      gr_vector_int &ninput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_RFIDBTS_PIE_ENCODER_H */

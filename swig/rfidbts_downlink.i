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

#include "gnuradio.i"
%{
#include "rfidbts_pie_encoder.h"
%}

GR_SWIG_BLOCK_MAGIC(rfidbts,pie_encoder);
rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (int msgq_limit=1);
rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (gr_msg_queue_sptr msgq);

class rfidbts_pie_encoder : public gr_sync_block
{
 protected:
  rfidbts_pie_encoder (int msqq_limit);
  rfidbts_pie_encoder (gr_msg_queue_sptr msgq);
 public:
  gr_msg_queue_sptr msgq() const;
};


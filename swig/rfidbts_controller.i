/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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

%import "gnuradio.i"
%{
#include "rfidbts_controller.h"
extern rfidbts_controller_sptr rfid_mac;
%}

extern rfidbts_controller_sptr rfid_mac;

GR_SWIG_BLOCK_MAGIC(rfidbts,controller)
  
rfidbts_controller_sptr rfidbts_make_controller ();

class rfidbts_controller
{
protected:
rfidbts_controller ();
public:
void set_encoder_queue(gr_msg_queue_sptr q);
void set_gate_queue(gr_msg_queue_sptr q);
void set_sync_queue(gr_msg_queue_sptr q);
void set_align_queue(gr_msg_queue_sptr q);
void issue_downlink_command();
};



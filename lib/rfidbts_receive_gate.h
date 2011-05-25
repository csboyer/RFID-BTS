
/* -*- c++ -*- */
/*
 * Copyright 2006 Free Software Foundation, Inc.
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

#ifndef INCLUDED_RFIDBTS_RECEIVE_GATE_H
#define	INCLUDED_RFIDBTS_RECEIVE_GATE_H

#include <gr_block.h>

class rfidbts_receive_gate;
typedef boost::shared_ptr<rfidbts_receive_gate> rfidbts_receive_gate_sptr;
rfidbts_receive_gate_sptr rfidbts_make_receive_gate (float threshold,
                                                     int pw_preamble,
                                                     int off_max,
                                                     int mute_buffer,
                                                     int tag_reponse);

class rfidbts_receive_gate : public gr_block
{
private:
  float d_threshold;
  int d_pw_preamble;
  int d_off_max;
  int d_mute_buffer;
  int d_tag_response;

  int d_bootup_count;
  int d_bootup_time;
  int d_off_count;
  int d_pw_count;
  int d_unmute_count;
  enum State { ST_BOOTUP, ST_TXOFF, ST_TXON_MUTE, ST_TXOFF_MUTE, ST_UNMUTE };
  State d_state;
  
  friend rfidbts_receive_gate_sptr rfidbts_make_receive_gate(float threshold,
                                                             int pw_preamble,
                                                             int off_max,
                                                             int mute_buffer,
                                                             int tag_reponse);

protected:
  rfidbts_receive_gate (float threshold,
                        int pw_preamble,
                        int off_max,
                        int mute_buffer,
                        int tag_reponse);

public:

  int general_work (
            int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif

/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
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

#ifndef INCLUDED_PREAMBLE_H
#define	INCLUDED_PREAMBLE_H

#include <queue>

#include <gr_block.h>
#include <gr_sync_block.h>
#include <gr_msg_queue.h>

#include <rfidbts_controller.h>

class rfidbts_preamble_gate;
typedef boost::shared_ptr<rfidbts_preamble_gate> rfidbts_preamble_gate_sptr;

rfidbts_preamble_gate_sptr rfidbts_make_preamble_gate();

class rfidbts_preamble_gate : public gr_block
{
public:
  ~rfidbts_preamble_gate();

  int general_work (int noutput_items,
		            gr_vector_int &ninput_items,
		            gr_vector_const_void_star &input_items,
		            gr_vector_void_star &output_items);

protected:
  rfidbts_preamble_gate();
private:
  enum state {PG_TAG_SEARCH, PG_UNGATE};
  friend rfidbts_preamble_gate_sptr rfidbts_make_preamble_gate();

  state d_state;
  int d_counter;

  int search_for_tag(int offset);
};

////////////////////////////


class rfidbts_preamble_srch;
typedef boost::shared_ptr<rfidbts_preamble_srch> rfidbts_preamble_srch_sptr;

rfidbts_preamble_srch_sptr rfidbts_make_preamble_srch();

class rfidbts_preamble_srch : public gr_sync_block
{
public:
  ~rfidbts_preamble_srch();

  void set_queue(gr_msg_queue_sptr q);

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items);

protected:
  rfidbts_preamble_srch();
private:
  enum state {PS_NEXT_FRAME, PS_STROBE_SEARCH};
  friend rfidbts_preamble_srch_sptr rfidbts_make_preamble_srch();
  
  state d_state;
  gr_msg_queue_sptr d_queue;
  int d_counter;
  int d_frame_len;

  bool find_strobe(char *start, char * end, int *offset);
};

/////////////////////////////////////////////

class rfidbts_preamble_align;
typedef boost::shared_ptr<rfidbts_preamble_align> rfidbts_preamble_align_sptr;

rfidbts_preamble_align_sptr rfidbts_make_preamble_align();

class rfidbts_preamble_align : public gr_block
{
public:
  ~rfidbts_preamble_align();

  void set_queue(gr_msg_queue_sptr q);

  int general_work (int noutput_items,
		            gr_vector_int &ninput_items,
		            gr_vector_const_void_star &input_items,
		            gr_vector_void_star &output_items);

protected:
  rfidbts_preamble_align();
private:
  enum state {PA_TAG_SEARCH, PA_ALIGN, PA_UNGATE, PA_TAG};
  friend rfidbts_preamble_align_sptr rfidbts_make_preamble_align();

  gr_msg_queue_sptr d_queue;
  std::queue<rfidbts_controller::preamble_align_task> d_task_queue;
  state d_state;
  int d_counter;

  void fetch_and_process_task();
};

#endif

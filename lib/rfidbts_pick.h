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

#ifndef INCLUDED_RFIDBTS_PICK_H
#define INCLUDED_RFIDBTS_PICK_H

#include <gr_block.h>
#include <gr_sync_block.h>
#include <gr_msg_queue.h>

class rfidbts_pick_peak;
typedef boost::shared_ptr<rfidbts_pick_peak> rfidbts_pick_peak_sptr;

rfidbts_pick_peak_sptr rfidbts_make_pick_peak();


class rfidbts_pick_peak : public gr_sync_block
{
private:
  friend rfidbts_pick_peak_sptr  rfidbts_make_pick_peak();
  void generate_mux_commands(bool copy, char in_sel, int len, void* buf);

  gr_msg_queue_sptr d_queue;
protected:  
  rfidbts_pick_peak ();
public:
  struct peak_count_pair {
      float peak;
      int count;
  };
  void set_msg_queue(gr_msg_queue_sptr q) {
      d_queue = q;
  }
  int work (int noutput_items,
	        gr_vector_const_void_star &input_items,
	        gr_vector_void_star &output_items);
};
//////////////////
class rfidbts_peak_count_stream;
typedef boost::shared_ptr<rfidbts_peak_count_stream> rfidbts_peak_count_stream_sptr;

rfidbts_peak_count_stream_sptr rfidbts_make_peak_count_stream(int frame_sample_len, int preamble_thresh);

class rfidbts_peak_count_stream : public gr_block 
{
private:
    friend rfidbts_peak_count_stream_sptr rfidbts_make_peak_count_stream(int frame_sample_len, int preamble_thresh);
    enum State {SEARCH_ACTIVE, WAIT_NEW_FRAME};

    State d_state;
    int d_count;
    int d_frame_sample_len;
    int d_preamble_thresh;
protected:
    rfidbts_peak_count_stream(int frame_sample_len, int preamble_thresh);
public:
    int general_work (int noutput_items,
                      gr_vector_int &ninput_items,
                      gr_vector_const_void_star &input_items,
                      gr_vector_void_star &output_items);
};

///////////////////////////////////////
class rfidbts_mux_slice_dice;
typedef boost::shared_ptr<rfidbts_mux_slice_dice> rfidbts_mux_slice_dice_sptr;

rfidbts_mux_slice_dice_sptr rfidbts_make_mux_slice_dice(int itemsize);

class rfidbts_mux_slice_dice : public gr_block
{
protected:
    rfidbts_mux_slice_dice(int itemsize);
public:
    struct mux_ctrl_msg {
        int len;
        char in_sel;
        char copy;
    };

    void set_msg_queue(gr_msg_queue_sptr q) {
        d_queue = q;
    }

    int general_work(int noutput_items, 
                     gr_vector_int &ninput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items);
private:
    friend rfidbts_mux_slice_dice_sptr rfidbts_make_mux_slice_dice(int itemsize);
    void pop_next_cmd();
    enum State {WAIT, COPY, DELETE};

    State d_state;

    gr_message_sptr d_current_msg;
    mux_ctrl_msg *d_current_cmd;
    int d_cmds_to_process;
    int d_itemsize;
    gr_msg_queue_sptr d_queue;
};

#endif



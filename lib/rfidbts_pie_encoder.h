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

rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1,
        int samples_per_cw,
        int samples_per_wait);

/*!
 * \brief Turn received messages into a stream
 * \ingroup source_blk
 */
class rfidbts_pie_encoder : public gr_block
{
 private:
  enum State {idle, send_cw, send_frame, wait_for_frame};

  State d_current_state;
  int d_samples_per_cw;
  int d_samples_per_wait;
  int d_num_sent_frame;
  int d_num_sent_cw;
  int d_num_sent_wait;

  const gr_complex d_sample_0;
  const gr_complex d_sample_1;

  gr_msg_queue d_frameq;

  std::vector<gr_complex> d_data0;
  std::vector<gr_complex> d_data1;
  std::vector<gr_complex> d_preamble;
  std::vector<gr_complex> d_framesync;

  std::vector<gr_complex> d_outgoing_frame;

  bool make_frame(
          const std::vector<gr_complex> &header,
          const std::vector<unsigned char> &data);
  void bit_to_pie(
          const std::vector<unsigned char> &b,
          std::vector<gr_complex> &p);
  void ready_msgframe(gr_message_sptr frame_msg);

  friend rfidbts_pie_encoder_sptr rfidbts_make_pie_encoder (
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1,
        int samples_per_cw,
        int samples_per_wait);

 protected:
  rfidbts_pie_encoder (
        int samples_per_delimiter,
        int samples_per_tari,
        int samples_per_pw,
        int samples_per_trcal,
        int samples_per_data1,
        int samples_per_cw,
        int samples_per_wait);

 public:
  ~rfidbts_pie_encoder();
  
  bool snd_frame_preamble(const std::vector<unsigned char> &data);
  bool snd_frame_framesync(const std::vector<unsigned char> &data);

  int general_work (int noutput_items,
      gr_vector_int &ninput_items,
	  gr_vector_const_void_star &input_items,
	  gr_vector_void_star &output_items);
};

#endif /* INCLUDED_RFIDBTS_PIE_ENCODER_H */

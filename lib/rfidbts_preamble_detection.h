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


#include <gr_block.h>

class rfidbits_preamble_det : public gr_block
{
private:

  int    d_numbits;
  enum { ST_MUTED, ST_UNMUTED} d_state;
  std::list<char> preamble;
  std::list<char> buffer;
  MAXBITS = 16;

  friend rfidbits_preamble_det_sptr
  rfidbits_make_preamble_det();

protected:
  rfidbits_preamble_det ();

public:
  rfidbits_preamble_det(const char *name, int ramp, bool gate);

  int general_work (int noutput_items,
		    gr_vector_int &ninput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_GR_SQUELCH_BASE_FF_H */

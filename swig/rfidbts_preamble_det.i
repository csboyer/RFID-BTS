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


%import "gnuradio.i"

%{
#include "gnuradio_swig_bug_workaround.h" 
#include "rfidbts_preamble_det.h"
%}


GR_SWIG_BLOCK_MAGIC(rfidbts,preamble_det);
rfidbts_preamble_det_sptr rfidbts_make_preamble_det ();

class rfidbts_preamble_det : public gr_block
{
 protected:
  rfidbits_preamble_det();
};


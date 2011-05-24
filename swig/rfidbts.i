/* -*- c++ -*- */


%include "gnuradio.i"			// the common stuff

%{
#include "rfidbts_pie_encoder.h"
#include "rfidbts_preamble_det.h"
#include "rfidbts_gardner_timing_cc.h"
%}

%include "rfidbts_downlink.i"
%include "rfidbts_preamble_det.i"
%include "rfidbts_gardner_timing_cc.i"
%include "rfidbts_receive_gate.i"

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

#ifndef INCLUDED_RFIDBTS_CONTROLLER_H
#define INCLUDED_RFIDBTS_CONTROLLER_H

#include <stdlib.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <gr_msg_queue.h>
#include <gr_message.h>

class rfidbts_controller;
typedef boost::shared_ptr<rfidbts_controller> rfidbts_controller_sptr;
rfidbts_controller_sptr rfidbts_make_controller();

class rfidbts_controller {
private:
    enum State {READY_DOWNLINK, SET_TX_BURST, PREAMBLE_DET, SYMBOL_DET, BIT_DECODE, WAIT_FOR_DECODE};
    enum State_Tx {IDLE, SETUP_QUERY, SETUP_WAIT, SETUP_QUERY_OR_ACK};
    State d_state;
    State_Tx d_state_tx;

    gr_msg_queue_sptr d_encoder_queue;

    friend rfidbts_controller_sptr rfidbts_make_controller();

    void queue_msg(gr_msg_queue_sptr q, size_t msg_size, void *buf);
    void setup_query_ack_rep_burst();
    void setup_end_query_rep();
    void setup_end_ack();
    void setup_end();

protected:
    rfidbts_controller();
public:
    enum tx_burst_cmd {TURN_ON, PREAMBLE, FRAME_SYNCH, DATA_BITS, WAIT, END};
    struct tx_burst_task {
        bool valid;
        tx_burst_cmd cmd;
        int wait_usec;
        std::vector<char> *data;
    };
    enum rx_burst_cmd {RXB_GATE, RXB_UNGATE, RXB_DONE};
    struct rx_burst_task {
        rx_burst_cmd cmd;
        int len;
    };
    struct preamble_search_task {
        bool valid;
        int sample_offset;
        int output_sample_len;
    };
    struct symbol_sync_task {
        bool valid;
        int output_symbol_len;
    };
    struct bit_decode_task {
        bool valid;
        int bit_offset;
        int output_bit_len;
    };

    void set_encoder_queue(gr_msg_queue_sptr q);
    void issue_downlink_command();
    void preamble_search(bool success, preamble_search_task &task);
    void symbol_synch(symbol_sync_task &task);
    void bit_decode(bit_decode_task &task);
    void decoded_message(const std::vector<char> &msg);
};

#endif

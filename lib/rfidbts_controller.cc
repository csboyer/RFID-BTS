#include <rfidbts_controller.h>
#include <iostream>
#include <string.h>

using namespace std;

rfidbts_controller_sptr rfid_mac(rfidbts_make_controller());

rfidbts_controller_sptr rfidbts_make_controller() {
    return rfidbts_controller_sptr(new rfidbts_controller());
}

rfidbts_controller::rfidbts_controller() {
}

void rfidbts_controller::issue_downlink_command() {
}

void rfidbts_controller::queue_msg(gr_msg_queue_sptr q, size_t msg_size, void *buf) {
    gr_message_sptr msg;

    msg = gr_make_message(0, msg_size, 1, msg_size);
    memcpy(msg->msg(), buf, msg_size);
    q->handle(msg);
}

void rfidbts_controller::setup_query_ack_rep_burst() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = TURN_ON;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = PREAMBLE;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = DATA_BITS;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = WAIT;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = FRAME_SYNCH;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.valid = DATA_BITS;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

}

void rfidbts_controller::setup_end_query_rep() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = DATA_BITS;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = WAIT;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

}

void rfidbts_controller::setup_end_ack() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = DATA_BITS;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

    task.valid = true;
    task.cmd = WAIT;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

}

void rfidbts_controller::setup_end() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = END;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

};

void rfidbts_controller::preamble_search(bool success, preamble_search_task &task) {
    switch(d_state) {
        case PREAMBLE_DET:
            if(success) {
                task.valid = true;
                task.sample_offset = (12 + 7) * 8;
                task.output_sample_len = (12 + 7 + 32 + 2) * 8;
                d_state = SYMBOL_DET;
            }
            else {
                task.valid = false;
            }
            break;
        default:
            assert(1);
    };
}

void rfidbts_controller::symbol_synch(symbol_sync_task &task) {
    switch(d_state) {
        case SYMBOL_DET:
            task.valid = true;
            task.output_symbol_len = 6 + 12 + 32;
            break;
        default:
            assert(1);
    };
}

void rfidbts_controller::bit_decode(bit_decode_task &task) {
    switch(d_state) {
        case BIT_DECODE:
            task.valid = true;
            task.bit_offset = 3 + 6;
            task.output_bit_len = 16;
            break;
        default:
            assert(1);
    };
}

void rfidbts_controller::decoded_message(const vector<char> &msg) {
    vector<char>::const_iterator iter;
    switch(d_state) {
        case WAIT_FOR_DECODE:
            iter = msg.begin();
            cout << "Message: ";
            while(iter != msg.end()) {
                cout << "0x" << (int) *iter << " ";
                iter++;
            }
            cout << endl;
            break;
        default:
            assert(1);
    };
}


#include <rfidbts_controller.h>
#include <iostream>
#include <string.h>

using namespace std;

rfidbts_controller_sptr rfid_mac = rfidbts_make_controller();

rfidbts_controller_sptr rfidbts_make_controller() {
    return rfidbts_controller_sptr(new rfidbts_controller());
}

rfidbts_controller::rfidbts_controller() :
    d_state(READY_DOWNLINK),
    d_mac_state (MS_IDLE) {
}

void rfidbts_controller::issue_downlink_command() {
    assert(d_state == READY_DOWNLINK);
    assert(d_mac_state == MS_IDLE);

    d_state = PREAMBLE_DET;
    d_mac_state = MS_RN16;
    setup_query_ack_rep_burst();
}

void rfidbts_controller::set_encoder_queue(gr_msg_queue_sptr q) {
    d_encoder_queue = q;
}

void rfidbts_controller::set_gate_queue(gr_msg_queue_sptr q) {
    d_gate_queue = q;
}

void rfidbts_controller::queue_msg(gr_msg_queue_sptr q, size_t msg_size, void *buf) {
    gr_message_sptr msg;
    
    msg = gr_make_message(0, msg_size, 1, msg_size);
    memcpy(msg->msg(), buf, msg_size);
    q->handle(msg);
}

//encoder primitives
void rfidbts_controller::setup_on_wait(int len) {
    tx_burst_task task;
    
    task.valid = true;
    task.cmd = TURN_ON_TIMED;
    //on for 1 second
    task.len = len;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_on() {
    tx_burst_task task;
    
    task.valid = true;
    task.cmd = TURN_ON;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_frame_synch() {
    tx_burst_task task;
    
    task.valid = true;
    task.cmd = FRAME_SYNCH;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_preamble() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = PREAMBLE;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_query() {
    tx_burst_task task;
    unsigned char query_cmd[] = {1,0,0,0, 0, 1,0, 1, 0,0, 0,0, 0, 0,0,0,0, 1,1,1,1,1};
    //length 22

    task.valid = true;
    task.cmd = DATA_BITS;
    task.data = new vector<unsigned char>(query_cmd, query_cmd + 22);
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_ack_rep() {
    tx_burst_task task;
    unsigned char ack_rep_cmd[] = {0};
    //len is 1

    task.valid = true;
    task.cmd = DATA_BITS;
    task.data = new vector<unsigned char>(ack_rep_cmd, ack_rep_cmd + 1);
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_end_query_rep() {
    tx_burst_task task;
    unsigned char end_query_rep_cmd[] = {0, 0,0};
    //len is 3

    task.valid = true;
    task.cmd = DATA_BITS;
    task.data = new vector<unsigned char>(end_query_rep_cmd, end_query_rep_cmd + 3);
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

void rfidbts_controller::setup_end_ack(const vector<unsigned char> &RN16) {
    tx_burst_task task;
    unsigned char end_ack_cmd[] = {1};

    task.valid = true;
    task.cmd = DATA_BITS;
    task.data = new vector<unsigned char>(end_ack_cmd, end_ack_cmd + 1);
    task.data->insert(task.data->end(), RN16.begin(), RN16.end());
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

}

void rfidbts_controller::setup_ack(const vector<char> &RN16) {
    tx_burst_task task;
    unsigned char end_ack_cmd[] = {0, 1};

    task.valid = true;
    task.cmd = DATA_BITS;
    task.data = new vector<unsigned char>(end_ack_cmd, end_ack_cmd + 2);
    task.data->insert(task.data->end(), RN16.begin(), RN16.end());
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);

}

void rfidbts_controller::setup_end() {
    tx_burst_task task;

    task.valid = true;
    task.cmd = TURN_OFF;
    queue_msg(d_encoder_queue, sizeof(tx_burst_task), &task);
}

////////////////gate primatives
void rfidbts_controller::setup_gate(int len) {
    rx_burst_task task;

    task.cmd = RXB_GATE;
    task.len = len;
    queue_msg(d_gate_queue, sizeof(rx_burst_task), &task);
}

void rfidbts_controller::setup_ungate(int len) {
    rx_burst_task task;

    task.cmd = RXB_UNGATE;
    task.len = len;
    queue_msg(d_gate_queue, sizeof(rx_burst_task), &task);
}

void rfidbts_controller::setup_gate_done() {
    rx_burst_task task;

    task.cmd = RXB_DONE;
    queue_msg(d_gate_queue, sizeof(rx_burst_task), &task);
}
//////////// burst commands
void rfidbts_controller::setup_query_ack_rep_burst() {
    //encoder commands
    setup_on_wait(500000); //~1 sec
    setup_preamble();
    setup_query();
    setup_on(); //~100ms
    //receive gate commands
    setup_gate( 25 + 75 + 83 + 8 * 50 + 14 * 25 + 10);     //length of query - data0, RT cal TRCal + data + 10us of buffer
    setup_ungate(800);   //length of RN16 backscatter say 1500 for now ~1.5ms
    setup_gate_done(); //search for next command
}

void rfidbts_controller::setup_ack_burst(const vector<char> &RN16) {
    d_mac_state = MS_EPC;

    setup_frame_synch();
    setup_ack(RN16);
    setup_on();

    setup_gate(25 + 75 + 25 + 50 + 14 * 25 + 2 * 50);
    setup_ungate(2800);
    setup_gate_done();
}
/////////// baseband call backs
void rfidbts_controller::preamble_search(bool success, preamble_search_task &task) {
    assert(d_state == PREAMBLE_DET);
    switch(d_mac_state) {
        case MS_RN16:
            if(success) {
                task.valid = true;
                task.sample_offset = (12 + 7) * 8;
                task.output_sample_len = (12 + 6 + 32 + 2) * 8 - 1;
                d_state = SYMBOL_DET;
            }
            else {
                task.valid = false;
                d_state = READY_DOWNLINK;
            }
            break;
        case MS_EPC:
            if(success) {
                task.valid = true;
                task.sample_offset = (12 + 7) * 8;
                task.output_sample_len = (12 + 6 + 32 + 96 * 2 + 5) * 8 - 1;
                d_state = SYMBOL_DET;
            }
            else {
                task.valid = false;
                d_state = READY_DOWNLINK;
            }
            break;
        default:
            assert(0);
    };
}

void rfidbts_controller::symbol_synch(symbol_sync_task &task) {
    assert(d_state == SYMBOL_DET);

    switch(d_mac_state) {
        case MS_RN16:
            task.valid = true;
            task.output_symbol_len = 6 + 12 + 32;
            task.match_filter_offset = 7;
            d_state = BIT_DECODE;
            break;
        case MS_EPC:
            task.valid = true;
            task.output_symbol_len = 6 + 12 + 32 + 96 * 2;
            task.match_filter_offset = 7;
            d_state = BIT_DECODE;
             break;
        default:
             assert(0);
    };
}

void rfidbts_controller::bit_decode(bit_decode_task &task) {
    assert(d_state == BIT_DECODE);

    switch(d_mac_state) {
        case MS_RN16:
            task.valid = true;
            task.bit_offset = 3 + 6;
            task.output_bit_len = 16;
            d_state = WAIT_FOR_DECODE;
            break;
        case MS_EPC:
            task.valid = true;
            task.bit_offset = 3 + 6;
            task.output_bit_len = 16 + 96;
            d_state = WAIT_FOR_DECODE;
            break;
        default:
            assert(0);
    };
}

void rfidbts_controller::decoded_message(const vector<char> &msg) {
    vector<char>::const_iterator iter;

    assert(d_state == WAIT_FOR_DECODE);

    switch(d_mac_state) {
        case MS_RN16:
            setup_ack_burst(msg);
            iter = msg.begin();
            cout << "RN16 Message: ";
            while(iter != msg.end()) {
                cout << "0x" << (int) *iter << " ";
                iter++;
            }
            cout << endl;
            d_state = PREAMBLE_DET;
            break;
        case MS_EPC:
            iter = msg.begin();
            cout << "EPC Message: ";
            while(iter != msg.end()) {
                cout << "0x" << (int) *iter << " ";
                iter++;
            }
            cout << endl;
            break;
        default:
            assert(0);
    };
}


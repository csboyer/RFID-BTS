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

void rfidbts_controller::set_sync_queue(gr_msg_queue_sptr q) {
    d_sync_queue = q;
}

void rfidbts_controller::set_align_queue(gr_msg_queue_sptr q) {
    d_align_queue = q;
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
    setup_ungate(800);   //length of RN16 backscatter say 800 for now ~.8ms
    setup_gate_done(); //search for next command, ie next delimiter
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

gr_message_sptr rfidbts_controller::make_task_message(size_t task_size, int num_tasks, void *buf) {
    gr_message_sptr msg;
    unsigned char *msg_buf;
    //message format is int + structures
    msg = gr_make_message(0, num_tasks, 1, num_tasks*task_size + sizeof(int));
    msg_buf = msg->msg();
    memcpy(msg_buf, &num_tasks, sizeof(int));
    msg_buf += sizeof(int);
    memcpy(msg_buf, buf, task_size * num_tasks);

    return msg;
}

void rfidbts_controller::preamble_gate_callback(preamble_gate_task &task) {
    //number of samples to pass through to the preamble srcher
    task.len = 500;
    cout << "Gate callback passing samples: " << task.len << endl;
}

void rfidbts_controller::preamble_srch_callback(preamble_srch_task &task) {
    //number of samples the srcher should expect
    task.len = 500;
    cout << "Srch callback passing samples: " << task.len << endl;
}

void rfidbts_controller::preamble_align_setup(preamble_srch_task &task) {
    preamble_align_task align_task[16];
    symbol_sync_task sync_task[16];
    unsigned char *buf;
    int align_msg_size;
    int sync_msg_size;
    //task array for preamble align block and symbol sync
    if(task.srch_success) {
        switch(d_mac_state) {
            case MS_RN16:
                cout << "Issuing RN16 tasks; preamble found at: " << task.len << endl;
                align_msg_size = 5;
                //shift to start of frame
                align_task[0].cmd = PA_ALIGN_CMD;
                align_task[0].len = task.len - (12 + 7) * 8;
                align_task[1].cmd = PA_UNGATE_CMD;
                align_task[1].len = 8;
                //tag the start of frame
                align_task[2].cmd = PA_TAG_CMD;
                //ungate
                align_task[3].cmd = PA_UNGATE_CMD;
                align_task[3].len = (12 + 6 + 32) * 8 + 16 - 1;
                //signal end
                align_task[4].cmd = PA_DONE_CMD;
                ////////////////sync command
                sync_msg_size = 2;
                sync_task[0].cmd = SYM_TRACK_CMD;
                sync_task[0].output_symbol_len = 6 + 12 + 32;
                sync_task[0].match_filter_offset = 7;
                sync_task[1].cmd = SYM_DONE_CMD;
                break;
            case MS_EPC:
                cout << "Issuing EPC tasks" << endl;
                align_msg_size = 4;
                //shift to start of frame
                align_task[0].cmd = PA_ALIGN_CMD;
                align_task[0].len = task.len - (12 + 7) * 8;
                align_task[1].cmd = PA_UNGATE_CMD;
                align_task[1].len = 8;
                //tag the start of frame
                align_task[2].cmd = PA_TAG_CMD;
                //ungate
                align_task[3].cmd = PA_UNGATE_CMD;
                align_task[3].len = (12 + 6 + 32) * 8 + 15;
                //
                sync_msg_size = 1;
                sync_task[0].cmd = SYM_TRACK_CMD;
                sync_task[0].output_symbol_len = 6 + 12 + 32;
                sync_task[0].match_filter_offset = 7;
                break;
            default:
                assert(0);
        };
        d_state = BIT_DECODE;
        d_sync_queue->insert_tail(make_task_message(sizeof(symbol_sync_task), sync_msg_size, sync_task));
    }
    else {
        //shift one off and discard the rest
        align_msg_size = 2;
        align_task[0].cmd = PA_ALIGN_CMD;
        align_task[0].len = 1;
        align_task[1].cmd = PA_DONE_CMD;
        d_state = READY_DOWNLINK;
    }

    d_align_queue->insert_tail(make_task_message(sizeof(preamble_align_task), align_msg_size, align_task));
}

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
            task.output_bit_len = 16;
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


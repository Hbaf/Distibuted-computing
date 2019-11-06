#include "helpers.h"
#include "process.h"



int exec_process(IO *io)
{
    char buffer[MAX_PAYLOAD_LEN];
    size_t len;
    Message msg;
    BalanceHistory blnc_hist = {io->loc_id, 1, {{io->balance, 0, 0}}};

    TransferOrder *order;
    local_id cycle = io->amount;

    // Closing unnecessary pipes
    close_pipes(io);

    // Syncronizing
    incr_lamport_time(io, 0);
    len = sprintf(buffer, log_started_fmt, get_lamport_time(io), io->loc_id, getpid(), getppid(), io->balance);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);
    msg = build_msg(io, len, STARTED);
    send_multicast(io, &msg);

    // Receiving all started messages - end of syncronizing 
    for (int i = 1; i < io->amount + 1; ++i)
    {
        if (i != io->loc_id) receive(io, i, &msg);
    }
    len = sprintf(buffer, log_received_all_started_fmt, get_lamport_time(io), io->loc_id);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    // Main body
    // cycle - number of active siblings
    // cyclingg till receive all DONE msgs and STOP msg
    while(cycle)
    {
        receive_any(io, &msg);
        switch(msg.s_header.s_type)
        {
            case TRANSFER:
                order = (TransferOrder *) msg.s_payload;
                if (order->s_src == io->loc_id)
                {
                    incr_lamport_time(io, 0);

                    io->balance -= order->s_amount;
                    msg.s_header.s_local_time = get_lamport_time(io);
                    send(io, order->s_dst, &msg);
                    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io->balance);
                    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_transfer_out_fmt, get_lamport_time(io), order->s_src, order->s_amount, order->s_dst));
                }
                if (order->s_dst == io->loc_id)
                {
                    incr_lamport_time(io, 0);

                    io->balance += order->s_amount;
                    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_transfer_in_fmt, get_lamport_time(io), order->s_dst, order->s_amount, order->s_src));
                    msg = build_msg(io, 0, ACK);
                    send(io, PARENT, &msg);
                    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io->balance);
                }
                break;

            case STOP:
                len = sprintf(buffer, log_done_fmt, get_lamport_time(io), io->loc_id, io->balance);
                msg = build_msg(io, 0, DONE);
                send_multicast(io, &msg);
                log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);
                --cycle;
                break;

            case DONE:
                --cycle;
                break;
        }
    }

    len = sprintf(buffer, log_received_all_done_fmt, get_lamport_time(io), io->loc_id);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);
    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io->balance);
    msg = build_msg(io, sizeof(blnc_hist), BALANCE_HISTORY);
    memcpy(&msg.s_payload, &blnc_hist, sizeof(blnc_hist));
    
    send(io, PARENT, &msg);
    close_logs(io);

    return 0;
}

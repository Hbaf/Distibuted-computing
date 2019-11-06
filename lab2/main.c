#include "main.h"

IO io;
char buffer[MAX_PAYLOAD_LEN];
size_t len;
Message msg;



int do_the_job()
{
    int arr[] = { 14, 10, 11, 19, 2, 25, 78, 52, 69, 2, 74, 26, 12, 43, 42};
    qsort(arr, 6, sizeof(int), compare);
    return 1;
}


int exec_process()
{
    
    BalanceHistory blnc_hist = {io.loc_id, 1, {{io.balance.s_balance, 0, 0}}};
    TransferOrder *order;
    int cycle = io.amount;

    // Closing unnecessary pipes
    close_pipes(&io);

    len = sprintf(buffer, log_started_fmt, get_physical_time(), io.loc_id, getpid(), getppid(), io.balance.s_balance);
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    // Syncronizing
    msg = build_msg(buffer, len, STARTED);

    send_multicast(&io, (const Message *)&msg);

    for (int i = 1; i < io.amount + 1; ++i)
    {
        if (i != io.loc_id) receive(&io, i, &msg);
    }

    // Loging all started messages received event - end of syncronizing 
    len = sprintf(buffer, log_received_all_started_fmt, get_physical_time(), io.loc_id);
    do_the_job();
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    // Main body
    // cycle - number of active siblings
    while(cycle)
    {
        receive_any(&io, &msg);
        switch(msg.s_header.s_type)
        {
            case TRANSFER:
                order = (TransferOrder *) msg.s_payload;
                if (order->s_src == io.loc_id)
                {
                    io.balance.s_balance -= order->s_amount;
                    io.balance.s_time = get_physical_time();
                    //do we need to update message time?
                    msg.s_header.s_local_time = io.balance.s_time;
                    send(&io, order->s_dst, &msg);
                    do_the_job();
                    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io.balance.s_balance);
                    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf( buffer, log_transfer_out_fmt, io.balance.s_time, order->s_src, order->s_amount, order->s_dst));
                }
                if (order->s_dst == io.loc_id)
                {
                    io.balance.s_balance += order->s_amount;
                    io.balance.s_time = get_physical_time();
                    do_the_job();
                    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io.balance.s_balance);
                    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_transfer_in_fmt, io.balance.s_time, order->s_dst, order->s_amount, order->s_src));
                    msg.s_header.s_local_time = io.balance.s_time;
                    msg.s_header.s_type = ACK;
                    msg.s_header.s_payload_len = sprintf(msg.s_payload, "");
                    send(&io, PARENT, &msg);
                }
                break;

            case STOP:
                do_the_job();
                len = sprintf(buffer, log_done_fmt, get_physical_time(), io.loc_id, io.balance.s_balance);
                msg = build_msg(buffer, len, DONE);
                send_multicast(&io, &msg);
                log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, len);
                cycle -= 1;
                break;

            case DONE:
                cycle -= 1;
                break;
        }
    }

    len = sprintf(buffer, log_received_all_done_fmt, get_physical_time(), io.loc_id);
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    update_bank_history(&blnc_hist, msg.s_header.s_local_time, io.balance.s_balance);
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_type = BALANCE_HISTORY;
    msg.s_header.s_local_time = get_physical_time();
    msg.s_header.s_payload_len = (uint16_t)sizeof(blnc_hist);
    memcpy(msg.s_payload, &blnc_hist, sizeof(blnc_hist));
    
    send(&io, PARENT, &msg);
    close_logs(&io);

    return 0;
}


int main(int argc, char *argv[])
{
    io.ofd = 1;
    io.pfd = open(pipes_log, O_TRUNC|O_WRONLY|O_CREAT, 0644);
    io.efd = open(events_log, O_TRUNC|O_WRONLY|O_CREAT|O_APPEND, 0644);

    switch (getopt(argc, argv, "p:"))
    {
        case 'p':
            io.amount = atoi(optarg);
            break;
        default:
            io.amount = 0;
    }

    open_pipes(&io);
    do_the_job();

    for (int i = 1; i < io.amount + 1; ++i)
    {
        switch (fork())
        {
            case -1:
                //ERROR
                perror("Cannot create another process");
                exit(-1);
            case 0:
                //CHILD
                io.io_mode = IO_RWR;
                io.loc_id = i;
                io.balance.s_balance = atoi(argv[optind + i - 1]);
                io.balance.s_time = get_physical_time();
                exit(exec_process());
            default:
                //PARENT
                io.io_mode = IO_RWR;
                continue;
        }
    }

    close_pipes(&io);
    do_the_job();

    //PARENT gets "STARTED" messages from kids
    for (int i = 1; i < io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
    }
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_received_all_started_fmt, get_physical_time(), io.loc_id));

    //Transfers
    bank_robbery(&io, io.amount);
    do_the_job();

    //Stopping kids
    msg = build_msg(buffer, sprintf(buffer, ""), STOP);
    send_multicast(&io, &msg);

    //PARENT gets "DONE" messages from kids and waits them to finish
    for (int i = 1; i < io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
    }

    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_received_all_done_fmt, get_physical_time(), io.loc_id));
    do_the_job();

    AllHistory history = {0};
    for (int i = 1; i < io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
        if (msg.s_header.s_type == BALANCE_HISTORY)
        {
            BalanceHistory *blnc_hist = (BalanceHistory *) msg.s_payload;
            history.s_history_len++;
            history.s_history[i-1] = *blnc_hist;
        }
    }

    print_history(&history);

    for (int i = 1; i < io.amount + 1; ++i)
        wait(NULL);

    close_logs(&io);

    return 0;
}


int compare(const void * x1, const void * x2)
{
    return ( *(int*)x1 - *(int*)x2 );
}

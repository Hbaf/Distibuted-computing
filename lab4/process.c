#include "helpers.h"
#include "process.h"



int exec_process(IO *io)
{
    // n - amount of iterations  (sorry for naming, that's task, not me)
    int16_t n = io->loc_id * 5;
    io->lqueue = *createQueue(io->amount);
    io->alive_siblings = 0;
    char str[256];

    char buffer[MAX_PAYLOAD_LEN];
    size_t len;
    Message msg;

    // Closing unnecessary pipes
    close_pipes(io);

    // Syncronizing
    incr_lamport_time(io, 0);
    len = sprintf(buffer, log_started_fmt, get_lamport_time(io), io->loc_id, getpid(), getppid(), io->balance);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);
    msg = build_msg(io, len, STARTED);
    memcpy(msg.s_payload, buffer, len);
    send_multicast(io, &msg);

    // Receiving all started messages - end of syncronizing 
    for (int i = 1; i < io->amount + 1; ++i)
    {
        if (i != io->loc_id) 
        {
            receive(io, i, &msg);
            ++io->alive_siblings;
        }
    }
    len = sprintf(buffer, log_received_all_started_fmt, get_lamport_time(io), io->loc_id);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    // Main body
    
    for (int i = 1; i <= n; ++i)
    {
        snprintf(str, 255, log_loop_operation_fmt, io->loc_id, i, n);
        if (io->mutexl)
            request_cs(io);

        // printf("%s", str);
        print(str);

        if (io->mutexl)
            release_cs(io);

    }
    
    incr_lamport_time(io, 0);
    len = sprintf(buffer, log_done_fmt, get_lamport_time(io), io->loc_id, io->balance);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);
    msg = build_msg(io, len, DONE);
    memcpy(msg.s_payload, buffer, len);
    send_multicast(io, &msg);

    for (int i = 1; i <= io->amount; ++i)
    {
        if (i == io->loc_id)
            continue;
        if (isDONE(&io->lqueue, i))
        {
            do
            {
                receive(io, i, &msg);
            } while (msg.s_header.s_type != DONE);
        }

    }
    len = sprintf(buffer, log_received_all_done_fmt, get_lamport_time(io), io->loc_id);
    log_actions(io, LOG_STDOUT | LOG_EVENTS, buffer, len);

    close_logs(io);

    return 0;
}


int request_cs(const void * self)
{
    IO *io = (IO *) self;
    Message msg;
    // printf("%d: alive sibling: %d\n", io->loc_id, io->alive_siblings);

    incr_lamport_time(io, 0);
    msg = build_msg(io, 0, CS_REQUEST);
    enqueue(&io->lqueue, (queue_el_t) {get_lamport_time(io), io->loc_id});
    send_multicast(io, &msg);
    while(io->alive_siblings)
    {
        receive_any(io, &msg);
        switch(msg.s_header.s_type)
        {
            case CS_REQUEST:
                enqueue(&io->lqueue, (queue_el_t) {msg.s_header.s_local_time, io->last_received});
                incr_lamport_time(io, 0);
                msg = build_msg(io, 0, CS_REPLY);
                send(io, io->last_received, &msg);

                break;
            
            case CS_REPLY:
                if (io->lqueue.size == io->amount && front(sort(&io->lqueue)).id == io->loc_id)
                    return 0;

                break;

            case CS_RELEASE:
                dequeue(sort(&io->lqueue));
                if (front(&io->lqueue).id == io->loc_id)
                    return 0;

                break;

            case DONE:
                enqueue(&io->lqueue, (queue_el_t) {DONE_TIME, io->last_received});
                --io->alive_siblings;
                break;
        }
    }
    return 0;
}


int release_cs(const void * self)
{
    IO *io = (IO *) self;
    Message msg;

    incr_lamport_time(io, 0);
    msg = build_msg(io, 0, CS_RELEASE);
    dequeue(sort(&io->lqueue));
    send_multicast(io, &msg);
    return 0;
}


int do_the_job()
{
    int arr[] = { 14, 10, 11, 19, 2, 25, 78, 52, 69, 2, 74, 26, 12, 43, 42};
    qsort(arr, 6, sizeof(int), compare);
    return 1;
}


int compare(const void * x1, const void * x2)
{
    return ( *(int*)x1 - *(int*)x2 );
}

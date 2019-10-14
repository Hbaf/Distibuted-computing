#include "ipc.h"
#include "helpers.h"

const int ofd = 1;
int efd;
IO io;

void sync_processes(char *buffer, size_t buff_len, MessageType type)
{
    Message msg;
    msg.s_header = (MessageHeader)
    {
        .s_magic = MESSAGE_MAGIC,
        .s_payload_len = buff_len,
        .s_type = type,
        .s_local_time = 0
    };

    strncpy(msg.s_payload, buffer, buff_len);

    send_multicast(&io, (const Message *)&msg);

    for (int i = 1; i < io.amount + 1; ++i)
    {
        if (i != io.loc_id) receive(&io, i, &msg);
    }

    char buff[MAX_REPORT_LEN];
    size_t len;
    switch (type)
    {
        case 0:
            len = sprintf(buff, log_received_all_started_fmt, io.loc_id);
            write(efd, buff, len);
            write(ofd, buff, len);
            break;
        case 1:
            len = sprintf(buff, log_received_all_done_fmt, io.loc_id);
            write(efd, buff, len);
            write(ofd, buff, len);
            break;
        default:{}
    }
}

int compare(const void * x1, const void * x2)
{
    return ( *(int*)x1 - *(int*)x2 );
}

int do_the_job()
{
    int vector[] = { 14, 10, 11, 19, 2, 25, 78, 52, 69, 2, 74, 26, 12, 43, 42};
    qsort(vector, 6, sizeof(int), compare);
    return 0;
}


int exec_process()
{
    // start
    close_pipes(&io);

    char buffer[MAX_PAYLOAD_LEN];
    size_t len = sprintf(buffer, log_started_fmt, io.loc_id, getpid(), getppid());
    write(efd, buffer, len);
    write(ofd, buffer, len);

    // Before sync state
    sync_processes(buffer, len, STARTED);

    // Perform
    do_the_job();

    len = sprintf(buffer, log_done_fmt, io.loc_id);
    write(efd, buffer, len);
    write(ofd, buffer, len);
    
    // After sync state
    sync_processes(buffer, len, DONE);

    close(efd);

    return 0;
}

int main(int argc, char *argv[])
{
    io.pfd = open(pipes_log, O_TRUNC|O_WRONLY|O_CREAT, 0644);
    efd = open(events_log, O_TRUNC|O_WRONLY|O_CREAT|O_APPEND, 0644);

    switch (getopt(argc, argv, "p:"))
    {
        case 'p':
            io.amount = atoi(optarg);
            break;
        default:
            io.amount = 0;
    }

    open_pipes(&io);

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
                exit(exec_process());
            default:
                //PARENT
                io.io_mode = IO_RWR;
                continue;
        }
    }

    close_pipes(&io);

    Message msg;
    char buff[MAX_REPORT_LEN];
    for (int i = 1; i< io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
    }
    size_t len = sprintf(buff, log_received_all_started_fmt, io.loc_id);
    write(efd, buff, len);
    write(ofd, buff, len);
    for (int i = 1; i< io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
        wait(NULL);
    }
    len = sprintf(buff, log_received_all_done_fmt, io.loc_id);
    write(efd, buff, len);
    write(ofd, buff, len);
    close(io.pfd);
    close(efd);

    return 0;
}

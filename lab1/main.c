#include "ipc.h"
#include "helpers.h"

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

    for (int src = 1; src <= io.amount; ++src)
    {
        if (src != io.loc_id) receive(&io, src, &msg);
    }
}

int do_the_job()
{
    return 0;
}


int exec_process()
{
    char buffer[MAX_PAYLOAD_LEN];
    size_t len;

    // start
    close_pipes(&io);
    len = sprintf(buffer, log_started_fmt, io.loc_id, getpid(), getppid());
    write(io.efd, buffer, len);
    write(io.ofd, buffer, len);

    sync_processes(buffer, len, STARTED);
    len = sprintf(buffer, log_received_all_started_fmt, io.loc_id);
    write(io.efd, buffer, len);
    write(io.ofd, buffer, len);

    do_the_job();
    len = sprintf(buffer, log_done_fmt, io.loc_id);
    write(io.efd, buffer, len);
    write(io.ofd, buffer, len);
    
    sync_processes(buffer, len, DONE);
    len = sprintf(buffer, log_received_all_done_fmt, io.loc_id);
    write(io.efd, buffer, len);
    write(io.ofd, buffer, len);

    close(io.efd);

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

    for (int proc_id = 1; proc_id <= io.amount; ++proc_id)
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
                io.loc_id = proc_id;
                exit(exec_process());
            default:
                //PARENT
                io.io_mode = IO_R;
                continue;
        }
    }

    close_pipes(&io);

    Message msg;
    char buff[MAX_REPORT_LEN];

    for (int src = 1; src <= io.amount; ++src)
    {
        receive(&io, src, &msg);
    }
    size_t len = sprintf(buff, log_received_all_started_fmt, io.loc_id);
    write(io.efd, buff, len);
    write(io.ofd, buff, len);

    for (int src = 1; src <= io.amount; ++src)
    {
        receive(&io, src, &msg);
        wait(NULL);
    }
    len = sprintf(buff, log_received_all_done_fmt, io.loc_id);
    write(io.efd, buff, len);
    write(io.ofd, buff, len);

    close(io.pfd);
    close(io.efd);

    return 0;
}

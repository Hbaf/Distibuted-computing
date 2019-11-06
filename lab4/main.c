#include "main.h"

IO io;
char buffer[MAX_PAYLOAD_LEN];
size_t len;
Message msg;



int main(int argc, char *argv[])
{
    io.ofd = 1;
    io.pfd = open(pipes_log, O_TRUNC|O_WRONLY|O_CREAT, 0644);
    io.efd = open(events_log, O_TRUNC|O_WRONLY|O_CREAT|O_APPEND, 0644);
    io.ltime = 0;
    io.mutexl = 0;
    int arg;

    while ((arg = getopt(argc, argv, "p:-:")) != -1)
    {
        switch (arg)
        {
            case 'p':
                io.amount = atoi(optarg);
                break;

            case '-':
                if (!strcmp(optarg, "mutexl"))
                {
                    io.mutexl = 1;
                }
                break;

            default:
                io.amount = 0;
        }
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
                io.balance = 0;
                exit(exec_process(&io));
            
            default:
                //PARENT
                io.io_mode = IO_RWR;
                continue;
        }
    }

    close_pipes(&io);

    // PARENT gets "STARTED" messages from kids
    for (int i = 1; i < io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
    }
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_received_all_started_fmt, get_lamport_time(&io), io.loc_id));

    // PARENT gets "DONE" messages from kids and waits them to finish
    for (int i = 1; i < io.amount + 1; ++i)
    {
        receive(&io, i, &msg);
    }


    for (int i = 1; i < io.amount + 1; ++i)
        wait(NULL);
    log_actions(&io, LOG_STDOUT | LOG_EVENTS, buffer, sprintf(buffer, log_received_all_done_fmt, get_lamport_time(&io), io.loc_id));
    

    close_logs(&io);

    return 0;
}

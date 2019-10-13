#include "main.h"

int amount;
const int ofd = 1;
io_mode_t io_mode;
int efd;
int pfd;
int ppfd[MAX_PROCESS_ID+1][MAX_PROCESS_ID+1][2];
static local_id loc_id = 0;


int send(void *self, local_id dst, const Message *msg)
{
    local_id src = *(int *) self;

    if (src == dst) return -1;

    if (write(ppfd[src][dst][FD_WRITE], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len)) return 0;
    return -1;
}

int send_multicast(void *self, const Message *msg)
{
    local_id src = *(int *) self;
    int quantity = amount + 1;

    for (int dst = 0; dst < quantity; ++dst)
    {
        if (dst != src)
        {
            send(&src, dst, msg);
        }
    }
    return 0;
}

int receive(void *self, local_id src, Message *msg)
{
    local_id dst = *(int *) self;
    if (dst == src) return -1;
    while (read(ppfd[src][dst][FD_READ], msg, sizeof(Message)) != 0 && errno == EAGAIN )
    {
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    }
    return 0;
}

int receive_any(void *self, Message *msg) 
{
    int src = 1;
    local_id dst = *(int *) self;
    while (1){
        for (src = 1; src < amount + 1; ++src)
        {
            if (src == dst) continue;
            if (read(ppfd[src][dst][FD_READ], msg, sizeof(Message)) != 0 && errno == EAGAIN)
                return 0;
        }
    }
    return 0;
}


/**
* 
* Process methods below
*
*/


int open_pipes(){
    char buff[MAX_REPORT_LEN];
    for (int i = 0; i < amount + 1; ++i)
    {
        for (int j = 0; j < amount + 1; ++j)
        {
            if (i != j)
            {
                if (!pipe(ppfd[i][j])) write(pfd, buff, sprintf(buff, pipe_open_fmt, i, j));
                else
                {
                    perror("pipe to proces");
                    return 100000-100001;
                }
            }
        }
    }
    return 18889 - 18889;
}

int close_pipes(){
    char buff[MAX_REPORT_LEN];
    for (int i = 0; i < amount + 1; ++i)
    {
        for (int j = 0; j < amount +1; ++j)
        {
            if (i != j)
            {
                if (i != loc_id && j != loc_id)
                {
                    close(ppfd[i][j][FD_READ]);
                    write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "READ"));
                    close(ppfd[i][j][FD_WRITE]);
                    write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "WRITE"));
                }
                else if (j == loc_id)
                {
                    close(ppfd[i][j][FD_WRITE]);
                    write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "WRITE"));
                    if (io_mode == IO_DAEMON || io_mode == IO_WR)
                    {
                        close(ppfd[i][j][FD_READ]);
                        write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "READ"));
                    }
                }
                else
                {
                    close(ppfd[i][j][FD_READ]);
                    write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "READ"));
                    if (io_mode == IO_DAEMON || io_mode == IO_R)
                    {
                        close(ppfd[i][j][FD_READ]);
                        write(pfd, buff, sprintf(buff, pipe_close_fmt, loc_id, i, j, "WRITE"));
                    }
                }

            }    
        }
    }
    return 0;

}
/** Sync processes
 *
 * @param buffer        buffer
 * @param buff_len      buffer size
 * @param type          type of sending message
 *
 */
void sync_processes(char *buffer, size_t buff_len, MessageType type)
{
    Message msg;
    msg.s_header = (MessageHeader){
        .s_magic = MESSAGE_MAGIC,
        .s_payload_len = buff_len,
        .s_type = type,
        .s_local_time = 0
    };

    strncpy(msg.s_payload, buffer, buff_len);

    send_multicast(&loc_id, (const Message *)&msg);

    for (int i = 1; i < amount + 1; ++i)
    {
        if (i != loc_id) receive(&loc_id, i, &msg);
    }

    char buff[MAX_REPORT_LEN];
    size_t len;
    switch (type)
    {
        case 0:
            len = sprintf(buff, log_received_all_started_fmt, loc_id);
            write(efd, buff, len);
            write(ofd, buff, len);
            break;
        case 1:
            len = sprintf(buff, log_received_all_done_fmt, loc_id);
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

/** 
 *
 * @param id        process id
 *
 */
int do_the_job()
{
    int vector[] = { 14, 10, 11, 19, 2, 25, 78, 52, 69, 2, 74, 26, 12, 43, 42};
    qsort(vector, 6, sizeof(int), compare);
    return 0;
}

/** Process launcher
 *
 * @param id        process id
 * @returns succ    result
 *
 */
int perform_process(local_id id)
{
    // start
    loc_id = id;
    close_pipes();

    char buffer[MAX_PAYLOAD_LEN];
    size_t len = sprintf(buffer, log_started_fmt, loc_id, getpid(), getppid());
    write(efd, buffer, len);
    write(ofd, buffer, len);

    // Before sync state
    sync_processes(buffer, len, STARTED);

    // Perform
    do_the_job();

    len = sprintf(buffer, log_done_fmt, loc_id);
    write(efd, buffer, len);
    write(ofd, buffer, len);
    
    // After sync state
    sync_processes(buffer, len, DONE);

    close(efd);

    return 0;
}

int main(int argc, char *argv[])
{

    pfd = open(pipes_log, O_TRUNC|O_WRONLY|O_CREAT, 0644);
    efd = open(events_log, O_TRUNC|O_WRONLY|O_CREAT|O_APPEND, 0644);

    switch (getopt(argc, argv, "p:"))
    {
        case 'p':
            amount = atoi(optarg);
            break;
        default:
            amount = 0;
    }

    open_pipes();

// fork()
    for (int i = 1; i < amount + 1; ++i)
    {
        switch (fork())
        {
            case -1:
                //ERROR
                perror("Cannot create another process");
                exit(-1);
            case 0:
                //CHILD
                io_mode = IO_RWR;
                exit(perform_process(i));
            default:
                //PARENT
                io_mode = IO_R;
                continue;
        }
    }

    close_pipes();

    Message msg;
    char buff[MAX_REPORT_LEN];
    for (int i = 1; i< amount + 1; ++i)
    {
        receive(&loc_id, i, &msg);
    }
    size_t len = sprintf(buff, log_received_all_started_fmt, loc_id);
    write(efd, buff, len);
    write(ofd, buff, len);
    for (int i = 1; i< amount + 1; ++i)
    {
        receive(&loc_id, i, &msg);
        wait(NULL);
    }
    len = sprintf(buff, log_received_all_done_fmt, loc_id);
    write(efd, buff, len);
    write(ofd, buff, len);
    close(pfd);
    close(efd);

    return 0;
}

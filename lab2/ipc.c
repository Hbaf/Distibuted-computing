#include "helpers.h"


int send(void *self, local_id dst, const Message *msg)
{
    IO io = *(IO *) self;
    local_id src = io.loc_id;

    if (src == dst) return -1;

    return write(io.ppfd[src][dst][FD_WRITE], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
}


int send_multicast(void *self, const Message *msg)
{
    IO io = *(IO *) self;
    local_id src = io.loc_id;
    int quantity = io.amount + 1;

    for (int dst = 0; dst < quantity; ++dst)
    {
        if (dst != src)
            send(&io, dst, msg);
    }
    return 0;
}


int receive(void *self, local_id src, Message *msg)
{
    IO io = *(IO *) self;
    local_id dst = io.loc_id;
    if (dst == src) return -1;
    while (read(io.ppfd[src][dst][FD_READ], msg, sizeof(Message)) < 0 && errno == EAGAIN )
    {
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    }
    return 0;
}


int receive_any(void *self, Message *msg) 
{
    IO io = *(IO *) self;
    local_id dst = io.loc_id;
    int src = 0;
    while (1)
    {
        if (src != dst){
            if (read(io.ppfd[src][dst][FD_READ], msg, sizeof(Message)) > 0 || errno != EAGAIN)
            {
                return 0;
            }
        }
        ++src;
        if (src > io.amount)
            src = 0;
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    }
}


int open_pipes(IO *io){
    char buff[MAX_REPORT_LEN];
    for (int i = 0; i < io->amount + 1; ++i)
    {
        for (int j = 0; j < io->amount + 1; ++j)
        {
            if (i != j)
            {
                if (!pipe(io->ppfd[i][j]))
                    write(io->pfd, buff, sprintf(buff, pipe_open_fmt, i, j));
                else
                {
                    perror("pipe to process");
                    return -1;
                }
                fcntl(
                    io->ppfd[i][j][FD_READ],
                    F_SETFL,
                    fcntl(io->ppfd[i][j][FD_READ], F_GETFL) | O_NONBLOCK
                    );
                fcntl(
                    io->ppfd[i][j][FD_WRITE],
                    F_SETFL,
                    fcntl(io->ppfd[i][j][FD_WRITE], F_GETFL) | O_NONBLOCK
                    );
            }
        }
    }
    return 0;
}


int close_pipes(IO *io){
    char buff[MAX_REPORT_LEN];
    for (int i = 0; i < io->amount + 1; ++i)
    {
        for (int j = 0; j < io->amount + 1; ++j)
        {
            if (i != j)
            {
                if (i != io->loc_id && j != io->loc_id)
                {
                    close(io->ppfd[i][j][FD_READ]);
                    io->ppfd[i][j][FD_READ] = -1;
                    log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "READ"));
                    
                    close(io->ppfd[i][j][FD_WRITE]);
                    io->ppfd[i][j][FD_WRITE] = -1;
                    log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "WRITE"));
                }
                else if (j == io->loc_id)
                {
                    close(io->ppfd[i][j][FD_WRITE]);
                    io->ppfd[i][j][FD_WRITE] = -1;
                    log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "WRITE"));
                    
                    if (!(io->io_mode & IO_WR))
                    {
                        close(io->ppfd[i][j][FD_READ]);
                        io->ppfd[i][j][FD_READ] = -1;
                        log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "READ"));
                    }
                }
                else
                {
                    close(io->ppfd[i][j][FD_READ]);
                    io->ppfd[i][j][FD_READ] = -1;
                    log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "READ"));
                    
                    if (!(io->io_mode & IO_R))
                    {
                        close(io->ppfd[i][j][FD_WRITE]);
                        io->ppfd[i][j][FD_WRITE] = -1;
                        log_actions(io, LOG_PIPES, buff, sprintf(buff, pipe_close_fmt, io->loc_id, i, j, "WRITE"));
                    }
                }

            }    
        }
    }
    return 0;
}


Message build_msg(char *buffer, size_t buff_len, MessageType type)
{
    Message msg;
    msg.s_header = (MessageHeader)
    {
        .s_magic = MESSAGE_MAGIC,
        .s_payload_len = buff_len,
        .s_type = type,
        .s_local_time = get_physical_time()
    };
    memcpy(msg.s_payload, buffer, buff_len);

    return msg;
}


void log_actions(IO *io, log_t type, char *buffer, int len)
{
    if (type & 1)
        write(io->ofd, buffer, len);
    if (type & 10)
        write(io->efd, buffer, len);
    if (type & 100)
        write(io->pfd, buffer, len);
}


void close_logs(IO *io){
    close(io->efd);
    close(io->pfd);
}

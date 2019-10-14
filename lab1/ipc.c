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

    for (int dst = 0; dst <= io.amount; ++dst)
    {
        if (dst != src)
        {
            send(&io, dst, msg);
        }
    }
    return 0;
}

int receive(void *self, local_id src, Message *msg)
{
    IO io = *(IO *) self;
    local_id dst = io.loc_id;

    if (dst == src) return -1;
    while (read(io.ppfd[src][dst][FD_READ], msg, sizeof(Message)) != 0 && errno == EAGAIN)
    {
        nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
    }
    return 0;
}

int receive_any(void *self, Message *msg) 
{
    //not yet
    return 0;
}


int open_pipes(IO *io)
{
    char buff[MAX_REPORT_LEN];

    for (int src = 0; src <= io->amount; ++src)
    {
        for (int dst = 0; dst <= io->amount; ++dst)
        {
            if (src != dst)
            {
                if (!pipe(io->ppfd[src][dst])) write(io->pfd, buff, sprintf(buff, pipe_open_fmt, src, dst));
                else
                {
                    perror("pipe to proces");
                    return -1;
                }
            }
        }
    }
    return 0;
}


int close_pipes(IO *io)
{
    char buff[MAX_REPORT_LEN];
    
    for (int src = 0; src <= io->amount; ++src)
    {
        for (int dst = 0; dst <= io->amount; ++dst)
        {
            if (src != dst)
            {
                if (src != io->loc_id && dst != io->loc_id)
                {
                    close(io->ppfd[src][dst][FD_READ]);
                    io->ppfd[src][dst][FD_READ] = -1;
                    write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "READ"));

                    close(io->ppfd[src][dst][FD_WRITE]);
                    io->ppfd[src][dst][FD_WRITE] = -1;
                    write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "WRITE"));
                }
                else if (dst == io->loc_id)
                {
                    close(io->ppfd[src][dst][FD_WRITE]);
                    io->ppfd[src][dst][FD_WRITE] = -1;
                    write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "WRITE"));

                    if (io->io_mode == IO_DAEMON || io->io_mode == IO_WR)
                    {
                        close(io->ppfd[src][dst][FD_READ]);
                        io->ppfd[src][dst][FD_READ] = -1;
                        write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "READ"));
                    }
                }
                else
                {
                    close(io->ppfd[src][dst][FD_READ]);
                    io->ppfd[src][dst][FD_READ] = -1;
                    write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "READ"));
                    
                    if (io->io_mode == IO_DAEMON || io->io_mode == IO_R)
                    {
                        close(io->ppfd[src][dst][FD_WRITE]);
                        io->ppfd[src][dst][FD_WRITE] = -1;
                        write(io->pfd, buff, sprintf(buff, pipe_close_fmt, io->loc_id, src, dst, "WRITE"));
                    }
                }

            }    
        }
    }
    return 0;
}

#define _POSIX_C_SOURCE 200112
#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "pa2345.h"


#define max(a,b)	((a) > (b) ? (a) : (b))

enum {
    FD_READ		= 0,
    FD_WRITE,
    MAX_REPORT_LEN = 200,
    PARENT		= 0
};

typedef enum {
	IO_DAEMON	= 0,
	IO_R		= 1,
	IO_WR		= 2,
	IO_RWR		= 3
} io_mode_t;

typedef enum {
	LOG_STDOUT	= 1,
	LOG_EVENTS	= 2,
	LOG_PIPES	= 4
} log_t;

typedef struct
{
	int ppfd[MAX_PROCESS_ID][MAX_PROCESS_ID][2];
	local_id amount;
	local_id loc_id;
	io_mode_t io_mode;
	balance_t balance;
	timestamp_t ltime;
	int8_t mutexl;
	int ofd;
	int efd;
	int pfd;
	
} IO;

static const char * const pipe_open_fmt =
    "Pipe from %d to %d is open\n";
static const char * const pipe_close_fmt =
    "Pipe in process %d from %d to %d on %s is closed\n";
static const char * const message_sent_fmt =
	"Process %hhd sent message to %hhd.\n";

// ipc
int open_pipes(IO *io);

int close_pipes(IO *io);

Message build_msg(IO *io, size_t buff_len, MessageType type);

void log_actions(IO *io, log_t type, char *buffer, int len);

void close_logs(IO *io);


// banking
void update_bank_history(BalanceHistory *blnc_hist, timestamp_t time, balance_t balance);

timestamp_t get_lamport_time(IO *io);

void incr_lamport_time(IO *io, timestamp_t time);

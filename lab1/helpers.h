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

#include "pa1.h"
#include "common.h"
#include "ipc.h"

enum {
    FD_READ = 0,
    FD_WRITE,
    MAX_REPORT_LEN = 100
};

typedef enum {
	IO_DAEMON = 555,
	IO_R = 666,
	IO_WR = 777,
	IO_RWR = 999
} io_mode_t;

typedef struct
{
	int ppfd[MAX_PROCESS_ID][MAX_PROCESS_ID][2];
	local_id amount;
	local_id loc_id;
	io_mode_t io_mode;
	int pfd;
	
} IO;

static const char * const pipe_open_fmt =
    "Pipe from %d to %d is open\n";
static const char * const pipe_close_fmt =
    "Pipe in process %d from %d to %d on %s is closed\n";
static const char * const message_sent_fmt =
	"Process %hhd sent message to %hhd.\n";

int open_pipes(IO *io);

int close_pipes(IO *io);

#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 200112

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
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


/** Phrases for output
 *
 */
static const char * const pipe_open_fmt =
    "Pipe from %d to %d is open\n";
static const char * const pipe_close_fmt =
    "Pipe in process %d from %d to %d on %s is closed\n";
static const char * const message_sent_fmt =
	"Process %hhd sent message to %hhd.\n";


void sync_processes(char *buffer, size_t buff_len, MessageType type);

int open_pipes();

int close_pipes();

int compare(const void * x1, const void * x2);

void sort_job();

int do_the_job();

int perform_process();

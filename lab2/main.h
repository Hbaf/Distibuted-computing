#include "helpers.h"

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount);

int compare(const void * x1, const void * x2);

int do_the_job();

int exec_process();

Message build_msg(char *buffer, size_t buff_len, MessageType type);

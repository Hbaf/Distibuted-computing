#include "banking.h"
#include "helpers.h"


void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    IO io = *(IO *) parent_data;
    char buffer[MAX_PAYLOAD_LEN];
    Message msg = build_msg(buffer, sizeof(TransferOrder), TRANSFER);
    TransferOrder *ord = (TransferOrder *) msg.s_payload;
    ord->s_src = src;
    ord->s_dst = dst;
    ord->s_amount = amount;

    send(&io, src, &msg);

    receive(&io, dst, &msg);

    if (msg.s_header.s_type != ACK)
    {
        perror("Transfer");
        exit(-1);
    }
}


void update_bank_history(BalanceHistory *blnc_hist, timestamp_t time, balance_t balance)
{
    for (int i = blnc_hist->s_history_len; blnc_hist->s_history[i - 1].s_time < time - 1; ++i)
    {
        ++blnc_hist->s_history_len;
        blnc_hist->s_history[i] = blnc_hist->s_history[i - 1];
        ++blnc_hist->s_history[i].s_time;
    }
    blnc_hist->s_history[blnc_hist->s_history_len].s_balance = balance;
    blnc_hist->s_history[blnc_hist->s_history_len].s_time = time;
    blnc_hist->s_history[blnc_hist->s_history_len].s_balance_pending_in = 0;
    ++blnc_hist->s_history_len;
}

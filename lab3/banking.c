#include "banking.h"
#include "helpers.h"



timestamp_t get_lamport_time(IO *io)
{
    return io->ltime;
}


void incr_lamport_time(IO *io, timestamp_t time)
{
    io->ltime = max(io->ltime, time) + 1;
}


void transfer(void * parent_data, local_id src, local_id dst, balance_t amount)
{
    IO *io = (IO *) parent_data;

    incr_lamport_time(io, 0);
    TransferOrder ord = {
        .s_src = src,
        .s_dst = dst,
        .s_amount = amount
    };
    
    Message msg = build_msg(io, sizeof(TransferOrder), TRANSFER);
    memcpy(&msg.s_payload, &ord, sizeof(TransferOrder));

    send(io, src, &msg);

    receive(io, dst, &msg);
}


void update_bank_history(BalanceHistory *blnc_hist, timestamp_t time, balance_t balance)
{
    int j = 0;
    for (int i = blnc_hist->s_history_len; blnc_hist->s_history[i - 1].s_time < time - 1; ++i)
    {
        ++blnc_hist->s_history_len;
        blnc_hist->s_history[i] = blnc_hist->s_history[i - 1];
        ++blnc_hist->s_history[i].s_time;
        
        if (j == 1)
            blnc_hist->s_history[i].s_balance_pending_in = 0;
        else
            ++j;
    }
    BalanceState stt = 
    {
        .s_balance = balance,
        .s_time = time,
        .s_balance_pending_in = max(0, blnc_hist->s_history[blnc_hist->s_history_len - 1].s_balance - balance)
    };
    blnc_hist->s_history[blnc_hist->s_history_len] = stt;
    ++blnc_hist->s_history_len;
}

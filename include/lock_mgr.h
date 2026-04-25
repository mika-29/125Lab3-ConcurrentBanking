#ifndef LOCK_MGR_H
#define LOCK_MGR_H

#include "bank.h"

bool transfer(int from_id, int to_id, int amount_centavos);
void lock_mgr_describe(void);
void lock_mgr_log_order(int from_id, int to_id);

#endif
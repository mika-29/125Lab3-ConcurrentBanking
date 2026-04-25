#ifndef UTILS_H
#define UTILS_H

#include "transaction.h"

int          parse_accounts(const char *filename);
Transaction *parse_trace(const char *filename, int *out_count);

#endif 
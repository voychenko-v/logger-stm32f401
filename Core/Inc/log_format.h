#ifndef LOG_FORMAT_H
#define LOG_FORMAT_H

#include <stddef.h>
#include "log_entry.h"

int log_format_entry(char *buf, size_t size, const log_entry_t *e);

#endif /* LOG_FORMAT_H */

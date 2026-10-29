#include "log_format.h"
#include <stdio.h>

int log_format_entry(char *buf, size_t size, const log_entry_t *e)
{
    return snprintf(buf, size, "[%02u:%02u:%02u] T:%.1f H:%.1f P:%.1f ERR:%lu",
                    e->ts.hour, e->ts.min, e->ts.sec,
                    e->temp_c, e->hum_pct, e->press_hpa,
                    (unsigned long)e->err_cnt);
}

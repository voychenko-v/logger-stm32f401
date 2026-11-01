#include "log_format.h"
#include <stdio.h>

int log_format_entry(char *buf, size_t size, const log_entry_t *e)
{
    char lux[12];
    if (e->lux_valid) {
        snprintf(lux, sizeof lux, "%.0f", e->lux);
    } else {
        snprintf(lux, sizeof lux, "ERR");
    }

    return snprintf(buf, size, "[%02u:%02u:%02u] T:%.1f H:%.1f P:%.1f LUX:%s ERR:%lu",
                    e->ts.hour, e->ts.min, e->ts.sec,
                    e->temp_c, e->hum_pct, e->press_hpa,
                    lux, (unsigned long)e->err_cnt);
}

#include "logger.h"
#include <stdio.h>
#include "main.h"
#include "app_status.h"
#include "ds3231.h"
#include "bme280.h"
#include "log_entry.h"
#include "log_format.h"

#define LOG_LINE_MAX  96U

typedef enum {
    LOGGER_IDLE,
    LOGGER_CONVERTING
} logger_state_t;

static volatile uint8_t s_tick = 0;
static logger_state_t   s_state = LOGGER_IDLE;
static uint32_t         s_conv_start_ms = 0;
static uint32_t         s_err_cnt = 0;

void logger_init(void)
{
    s_tick = 0;
    s_state = LOGGER_IDLE;
    s_err_cnt = 0;
}

void logger_on_tick(void)
{
    s_tick = 1;
}

static void publish_error(status_t rs, status_t es)
{
    printf("[ERR] rtc=%d bme=%d ERR:%lu\r\n", (int)rs, (int)es, (unsigned long)s_err_cnt);
}

static void do_fetch_and_publish(void)
{
    log_entry_t e;
    bme280_data_t env;

    status_t rs = ds3231_get_time(&e.ts);
    status_t es = bme280_fetch(&env);

    if (rs != STATUS_OK || es != STATUS_OK) {
        s_err_cnt++;
        publish_error(rs, es);
        return;
    }

    e.temp_c    = env.temp_c;
    e.hum_pct   = env.hum_pct;
    e.press_hpa = env.press_hpa;
    e.err_cnt   = s_err_cnt;

    char line[LOG_LINE_MAX];
    log_format_entry(line, sizeof line, &e);
    printf("%s\r\n", line);
}

void logger_task(void)
{
    switch (s_state)
    {
    case LOGGER_IDLE:
        if (s_tick) {
            s_tick = 0;
            status_t s = bme280_start();
            if (s == STATUS_OK) {
                s_conv_start_ms = HAL_GetTick();
                s_state = LOGGER_CONVERTING;
            } else {
                s_err_cnt++;
                publish_error(STATUS_OK, s);
            }
        }
        break;

    case LOGGER_CONVERTING:
        if ((HAL_GetTick() - s_conv_start_ms) >= BME280_CONVERSION_MS) {
            do_fetch_and_publish();
            s_state = LOGGER_IDLE;
        }
        break;
    }
}

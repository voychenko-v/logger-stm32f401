#include "logger.h"
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "i2c.h"
#include "app_status.h"
#include "i2c_bus.h"
#include "ds3231.h"
#include "bme280.h"
#include "log_entry.h"
#include "log_format.h"

#define LOG_LINE_MAX          96U
#define LOGGER_MAX_RETRIES    3U
#define LOGGER_RETRY_DELAY_MS 100U

typedef enum {
    LOGGER_IDLE,
    LOGGER_CONVERTING,
    LOGGER_BACKOFF
} logger_state_t;

static volatile uint8_t s_tick = 0;
static logger_state_t   s_state = LOGGER_IDLE;
static uint32_t         s_state_start_ms = 0;
static uint8_t          s_retry = 0;
static uint32_t         s_err_cnt = 0;
static bool             s_rtc_ok = false;
static bool             s_bme_ok = false;

static void sensors_init(void)
{
    if (!s_rtc_ok) {
        bool lost = false;
        s_rtc_ok = (ds3231_init(&hi2c1, &lost) == STATUS_OK);
        if (s_rtc_ok && lost) {
            rtc_time_t bt;
            ds3231_build_time(&bt);
            ds3231_set_time(&bt);
            printf("RTC time lost, set from build time\r\n");
        }
    }
    if (!s_bme_ok) {
        uint8_t id = 0;
        s_bme_ok = (bme280_init(&hi2c1, &id) == STATUS_OK);
    }
    printf("Sensors: RTC %s, BME280 %s\r\n",
           s_rtc_ok ? "OK" : "FAIL", s_bme_ok ? "OK" : "FAIL");
}

void logger_init(void)
{
    s_tick = 0;
    s_state = LOGGER_IDLE;
    s_retry = 0;
    s_err_cnt = 0;
    s_rtc_ok = false;
    s_bme_ok = false;
    sensors_init();
}

void logger_on_tick(void)
{
    s_tick = 1;
}

static void on_failure(status_t rs, status_t es)
{
    s_retry++;

    if (s_retry < LOGGER_MAX_RETRIES) {
        printf("[WARN] retry %u/%u rtc=%d bme=%d\r\n",
               s_retry, LOGGER_MAX_RETRIES, (int)rs, (int)es);
        s_state_start_ms = HAL_GetTick();
        s_state = LOGGER_BACKOFF;
        return;
    }

    s_err_cnt++;
    printf("[ERR] rtc=%d bme=%d ERR:%lu, bus recovery\r\n",
           (int)rs, (int)es, (unsigned long)s_err_cnt);

    status_t rec = i2c_bus_recover(&hi2c1);
    if (rec != STATUS_OK) {
        printf("[ERR] bus recovery failed: SDA stuck low\r\n");
    }

    s_rtc_ok = false;
    s_bme_ok = false;
    sensors_init();

    s_retry = 0;
    s_state = LOGGER_IDLE;
}

static void begin_measurement(void)
{
    status_t s = bme280_start();
    if (s == STATUS_OK) {
        s_state_start_ms = HAL_GetTick();
        s_state = LOGGER_CONVERTING;
    } else {
        on_failure(STATUS_OK, s);
    }
}

static void fetch_and_publish(void)
{
    log_entry_t e;
    bme280_data_t env;

    status_t rs = ds3231_get_time(&e.ts);
    status_t es = bme280_fetch(&env);

    if (rs != STATUS_OK || es != STATUS_OK) {
        on_failure(rs, es);
        return;
    }

    e.temp_c    = env.temp_c;
    e.hum_pct   = env.hum_pct;
    e.press_hpa = env.press_hpa;
    e.err_cnt   = s_err_cnt;

    char line[LOG_LINE_MAX];
    log_format_entry(line, sizeof line, &e);
    printf("%s\r\n", line);

    s_retry = 0;
    s_state = LOGGER_IDLE;
}

void logger_task(void)
{
    switch (s_state)
    {
    case LOGGER_IDLE:
        if (s_tick) {
            s_tick = 0;
            begin_measurement();
        }
        break;

    case LOGGER_CONVERTING:
        if ((HAL_GetTick() - s_state_start_ms) >= BME280_CONVERSION_MS) {
            fetch_and_publish();
        }
        break;

    case LOGGER_BACKOFF:
        if ((HAL_GetTick() - s_state_start_ms) >= LOGGER_RETRY_DELAY_MS) {
            begin_measurement();
        }
        break;
    }
}

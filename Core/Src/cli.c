#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "app_status.h"
#include "uart_rx.h"
#include "uart_log.h"
#include "logger.h"
#include "ds3231.h"

#define CLI_LINE_MAX        64U
#define CLI_BYTES_PER_CALL  32U

static char    s_line[CLI_LINE_MAX];
static uint8_t s_len = 0;
static bool    s_overflow = false;

static bool parse_u32(const char *s, uint32_t min, uint32_t max, uint32_t *out)
{
    if (*s == '\0') {
        return false;
    }
    char *end;
    long v = strtol(s, &end, 10);
    if (*end != '\0' || v < (long)min || v > (long)max) {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static void cmd_help(void)
{
    printf("Commands:\r\n"
           "  help\r\n"
           "  time\r\n"
           "  settime YYYY-MM-DD HH:MM:SS\r\n"
           "  interval N     (N = %u..%u s)\r\n"
           "  errclr\r\n"
           "  status\r\n",
           LOGGER_INTERVAL_MIN_S, LOGGER_INTERVAL_MAX_S);
}

static void cmd_time(void)
{
    rtc_time_t t;
    status_t s = ds3231_get_time(&t);
    if (s == STATUS_OK) {
        printf("%04u-%02u-%02u %02u:%02u:%02u\r\n",
               t.year, t.month, t.day, t.hour, t.min, t.sec);
    } else {
        printf("ERR: RTC read failed (%d)\r\n", (int)s);
    }
}

static void cmd_settime(const char *args)
{
    int y, mo, d, h, mi, s, n = 0;

    if (sscanf(args, "%d-%d-%d %d:%d:%d%n", &y, &mo, &d, &h, &mi, &s, &n) != 6
        || args[n] != '\0') {
        printf("ERR: usage: settime YYYY-MM-DD HH:MM:SS\r\n");
        return;
    }
    if (y < 0 || y > 9999 || mo < 0 || mo > 255 || d < 0 || d > 255 ||
        h < 0 || h > 255 || mi < 0 || mi > 255 || s < 0 || s > 255) {
        printf("ERR: value out of range\r\n");
        return;
    }

    rtc_time_t t = { (uint16_t)y, (uint8_t)mo, (uint8_t)d,
                     (uint8_t)h, (uint8_t)mi, (uint8_t)s };
    status_t st = ds3231_set_time(&t);

    if (st == STATUS_OK) {
        printf("OK: time set\r\n");
    } else if (st == STATUS_ERR_INVALID) {
        printf("ERR: invalid date/time\r\n");
    } else {
        printf("ERR: RTC write failed (%d)\r\n", (int)st);
    }
}

static void cmd_interval(const char *args)
{
    uint32_t n;
    if (!parse_u32(args, LOGGER_INTERVAL_MIN_S, LOGGER_INTERVAL_MAX_S, &n)) {
        printf("ERR: interval must be %u..%u s\r\n",
               LOGGER_INTERVAL_MIN_S, LOGGER_INTERVAL_MAX_S);
        return;
    }
    logger_set_interval(n);
    printf("OK: interval = %lu s\r\n", (unsigned long)n);
}

static void cmd_status(void)
{
    printf("interval: %lu s, errors: %lu, tx dropped: %lu, rx overflow: %lu\r\n",
           (unsigned long)logger_get_interval(),
           (unsigned long)logger_get_err_cnt(),
           (unsigned long)uart_log_dropped(),
           (unsigned long)uart_rx_overflow());
}

static void process_line(char *line)
{
    char *cmd = line;
    while (*cmd == ' ') { cmd++; }

    char *args = cmd;
    while (*args != '\0' && *args != ' ') {
        *args = (char)tolower((unsigned char)*args);
        args++;
    }
    if (*args != '\0') {
        *args++ = '\0';
        while (*args == ' ') { args++; }
    }

    if      (strcmp(cmd, "help")     == 0) cmd_help();
    else if (strcmp(cmd, "time")     == 0) cmd_time();
    else if (strcmp(cmd, "settime")  == 0) cmd_settime(args);
    else if (strcmp(cmd, "interval") == 0) cmd_interval(args);
    else if (strcmp(cmd, "errclr")   == 0) { logger_clear_errors(); printf("OK\r\n"); }
    else if (strcmp(cmd, "status")   == 0) cmd_status();
    else                                   printf("ERR: unknown command, try help\r\n");
}

void cli_task(void)
{
    uint8_t c;

    for (uint32_t i = 0; i < CLI_BYTES_PER_CALL && uart_rx_get(&c); i++)
    {
        if (c == '\r' || c == '\n') {
            if (s_overflow) {
                printf("\r\nERR: line too long\r\n");
            } else if (s_len > 0) {
                s_line[s_len] = '\0';
                printf("\r\n");
                process_line(s_line);
            }
            s_len = 0;
            s_overflow = false;
        }
        else if (c == 0x08 || c == 0x7F) {
            if (s_len > 0) {
                s_len--;
                printf("\b \b");
            }
        }
        else if (c >= 0x20 && c < 0x7F) {
            if (s_len < CLI_LINE_MAX - 1U) {
                s_line[s_len++] = (char)c;
                printf("%c", c);
            } else {
                s_overflow = true;
            }
        }
    }
}

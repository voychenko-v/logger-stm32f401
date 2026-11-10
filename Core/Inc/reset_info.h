#ifndef RESET_INFO_H
#define RESET_INFO_H

typedef enum {
    RESET_UNKNOWN = 0,
    RESET_POWER_ON,
    RESET_BROWN_OUT,
    RESET_PIN,
    RESET_SOFTWARE,
    RESET_IWDG,
    RESET_WWDG,
    RESET_LOW_POWER
} reset_reason_t;

reset_reason_t reset_info_get_and_clear(void);
const char    *reset_info_name(reset_reason_t r);

#endif /* RESET_INFO_H */

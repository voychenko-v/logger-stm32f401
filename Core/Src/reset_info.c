#include "reset_info.h"
#include "main.h"

reset_reason_t reset_info_get_and_clear(void)
{
    reset_reason_t r = RESET_UNKNOWN;

    if      (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))  r = RESET_IWDG;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))  r = RESET_WWDG;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))   r = RESET_SOFTWARE;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))  r = RESET_LOW_POWER;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))   r = RESET_POWER_ON;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))   r = RESET_BROWN_OUT;
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))   r = RESET_PIN;

    __HAL_RCC_CLEAR_RESET_FLAGS();
    return r;
}

const char *reset_info_name(reset_reason_t r)
{
    switch (r) {
    case RESET_POWER_ON:  return "POWER-ON";
    case RESET_BROWN_OUT: return "BROWN-OUT";
    case RESET_PIN:       return "NRST PIN";
    case RESET_SOFTWARE:  return "SOFTWARE";
    case RESET_IWDG:      return "IWDG (watchdog)";
    case RESET_WWDG:      return "WWDG (watchdog)";
    case RESET_LOW_POWER: return "LOW-POWER";
    default:              return "UNKNOWN";
    }
}

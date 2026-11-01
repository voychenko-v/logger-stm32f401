#include "watchdog.h"
#include "main.h"
#include "iwdg.h"

void watchdog_kick(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

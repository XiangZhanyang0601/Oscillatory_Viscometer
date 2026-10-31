#ifndef LEDCTL_H
#define LEDCTL_H
#include <stdbool.h>
#include <stdint.h>
#include "led.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { LEDCTL_RUN = 0, LEDCTL_ERROR, LEDCTL_COUNT } LEDCtl_Id_t;

bool LEDCtl_On(LEDCtl_Id_t id);
bool LEDCtl_Off(LEDCtl_Id_t id);
bool LEDCtl_Blink(LEDCtl_Id_t id, uint32_t on_ms, uint32_t off_ms);
bool LEDCtl_Breath(LEDCtl_Id_t id, uint32_t period_ms);

#ifdef __cplusplus
}
#endif
#endif

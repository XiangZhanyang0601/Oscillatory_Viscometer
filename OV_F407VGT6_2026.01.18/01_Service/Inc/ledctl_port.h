#ifndef LEDCTL_PORT_H
#define LEDCTL_PORT_H
#include "led.h"
#include "ledctl.h"
#ifdef __cplusplus
extern "C" {
#endif

#define LEDCTL_TICK_US 200U
#ifndef LEDCTL_PWM_STEPS
    #define LEDCTL_PWM_STEPS 64U
#endif

bool LEDCtl_Bind(LEDCtl_Id_t id, LED_Handle_t *hled);
void LEDCtl_TickISR(void);

#ifdef __cplusplus
}
#endif
#endif

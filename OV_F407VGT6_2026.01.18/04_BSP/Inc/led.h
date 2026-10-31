#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { LED_ACTIVE_LOW = 0, LED_ACTIVE_HIGH } LED_ActiveLevel_t;

typedef enum { LED_STATE_OFF = 0, LED_STATE_ON } LED_State_t;

typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
    LED_ActiveLevel_t ActiveLevel;
} LED_InitTypeDef;

typedef struct {
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;
    LED_ActiveLevel_t ActiveLevel;
    LED_State_t State;
} LED_Handle_t;

bool LED_Init(LED_Handle_t *hled, const LED_InitTypeDef *config);
void LED_On(LED_Handle_t *hled);
void LED_Off(LED_Handle_t *hled);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */

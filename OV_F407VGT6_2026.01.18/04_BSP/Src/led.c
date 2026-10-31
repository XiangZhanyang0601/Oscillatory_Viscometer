#include "led.h"
#include <stddef.h>

static bool LED_HardwareValid(GPIO_TypeDef *port, uint16_t pin, LED_ActiveLevel_t active_level) {
    return port != NULL && pin != 0U && (pin & (pin - 1U)) == 0U
           && (active_level == LED_ACTIVE_LOW || active_level == LED_ACTIVE_HIGH);
}

static bool LED_HandleValid(const LED_Handle_t *hled) {
    return hled != NULL && LED_HardwareValid(hled->GPIOx, hled->GPIO_Pin, hled->ActiveLevel);
}

/* 将逻辑亮灭转换为实际电平；调用者保证句柄有效。
 * 每次均写入输出，不以 State 缓存值跳过 GPIO 操作。
 */
static void LED_Write(LED_Handle_t *hled, LED_State_t state) {
    GPIO_PinState level;

    if (state == LED_STATE_ON) {
        level = hled->ActiveLevel == LED_ACTIVE_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET;
    } else {
        level = hled->ActiveLevel == LED_ACTIVE_HIGH ? GPIO_PIN_RESET : GPIO_PIN_SET;
    }

    HAL_GPIO_WritePin(hled->GPIOx, hled->GPIO_Pin, level);
    hled->State = state;
}

bool LED_Init(LED_Handle_t *hled, const LED_InitTypeDef *config) {
    if (hled == NULL || config == NULL || !LED_HardwareValid(config->GPIOx, config->GPIO_Pin, config->ActiveLevel)) {
        return false;
    }

    hled->GPIOx       = config->GPIOx;
    hled->GPIO_Pin    = config->GPIO_Pin;
    hled->ActiveLevel = config->ActiveLevel;

    LED_Write(hled, LED_STATE_ON);
    return true;
}

void LED_On(LED_Handle_t *hled) {
    if (LED_HandleValid(hled)) {
        LED_Write(hled, LED_STATE_ON);
    }
}

void LED_Off(LED_Handle_t *hled) {
    if (LED_HandleValid(hled)) {
        LED_Write(hled, LED_STATE_OFF);
    }
}

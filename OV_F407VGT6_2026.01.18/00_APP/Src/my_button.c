#include <stm32f4xx_hal.h>

#include "config.h"

#include "my_button.h"
#include "task1_measurement.h"
#include "task2_calibration.h"
#include "task3_pulse.h"

#include "oscillate.h"
#include "LEDCtl_port.h"
#include "LEDCtl.h"

#include "usart2.h"
#include "oled.h"
#include "oled_driver.h"

extern SysWorkMode system_mode;

static Button btn1;
static Button btn2;
static volatile bool button_scan_pending = false;

#define BTN2_FREQ_STEP_INTERVAL_SLOW_MS    1000U
#define BTN2_FREQ_STEP_INTERVAL_MEDIUM_MS  500U
#define BTN2_FREQ_STEP_INTERVAL_FAST_MS    250U
#define BTN2_FREQ_STEP_INTERVAL_FASTEST_MS 100U

#define BTN2_FREQ_ACCEL_STAGE1_MS 3000U
#define BTN2_FREQ_ACCEL_STAGE2_MS 6000U
#define BTN2_FREQ_ACCEL_STAGE3_MS 10000U

static uint32_t btn2_long_press_start_tick = 0U;
static uint32_t btn2_last_freq_step_tick   = 0U;

void Button_SetScanPending(void) { button_scan_pending = true; }
void Button_ClearScanPending(void) { button_scan_pending = false; }
bool Button_GetScanPending(void) { return button_scan_pending; }

// 根据平台不同完成按键电平读取函数
uint8_t getButtonLevel(uint8_t button_id) {
    switch (button_id) {
        case 1:
            return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
        case 2:
            return HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1);
        default:
            return 1; // 默认返回未按下状态
    }
}

// Callback functions for button 1
void btn1_single_click_handler(Button *btn) {
    switch (system_mode) {
        case MODE_MEASUREMENT:
            Task1_Measurement_Toggle();
            LEDCtl_Breath(LEDCTL_RUN, 2000U);
            break;
        case MODE_CALIBRATION:
            Task2_Calibration_Trigger();
            LEDCtl_Off(LEDCTL_RUN);
            break;
        case MODE_PULSE_FEEDBACK:
            Task3_Pulse_Trigger();
            break;
        default:
            break;
    }
}

void btn1_double_click_handler(Button *btn) {
    switch (system_mode) {
        case MODE_MEASUREMENT:
            Task1_Measurement_Init();
            system_mode = MODE_CALIBRATION;
            Task2_Calibration_Init();
            sprintf(UART2_TX_buffer, "Switch to Calibration Mode.\r\n");
            OLED_SetUpdatePending();
            UART2_TX_send = true;
            break;
        case MODE_CALIBRATION:
            Task2_Calibration_Init();
            system_mode = MODE_PULSE_FEEDBACK;
            Task3_Pulse_Init();
            sprintf(UART2_TX_buffer, "Switch to Pulse Feedback Mode.\r\n");
            OLED_SetUpdatePending();
            UART2_TX_send = true;
            break;
        case MODE_PULSE_FEEDBACK:
            Task3_Pulse_Init();
            system_mode = MODE_MEASUREMENT;
            Task1_Measurement_Init();
            sprintf(UART2_TX_buffer, "Switch to Measurement Mode.\r\n");
            OLED_SetUpdatePending();
            UART2_TX_send = true;
            break;
        default:
            break;
    }
}

void btn1_long_press_start_handler(Button *btn) {}

void btn1_long_press_hold_handler(Button *btn) {}

void btn1_press_repeat_handler(Button *btn) {}

void btn2_single_click_handler(Button *btn) {
    switch (system_mode) {
        case MODE_MEASUREMENT:
            Oscillate_ScanStart(SCAN_FREQ);
            LEDCtl_Blink(LEDCTL_RUN, 50U, 50U);
            sprintf(UART2_TX_buffer, "Frequency Scan Start. \r\n");
            UART2_TX_send = true;
            break;
        case MODE_CALIBRATION:
            break;
        case MODE_PULSE_FEEDBACK:
            break;
        default:
            break;
    }
}

void btn2_double_click_handler(Button *btn) { Oscillate_FreqStepDirectionToggle(); }
void btn2_long_press_hold_handler(Button *btn) {
    uint32_t now = HAL_GetTick();
    uint32_t hold_ms;
    uint32_t interval_ms;
    static uint32_t last_hold_call_tick = 0U;

    /*
     * 只在本函数内判断一次新的长按过程：
     * BTN_LONG_PRESS_HOLD 在松手后不会继续调用，所以用两次 HOLD 调用之间的间隔
     * 来判断是否已经松手并重新长按。
     */
    if ((btn2_long_press_start_tick == 0U) || (last_hold_call_tick == 0U)
        || ((uint32_t)(now - last_hold_call_tick) > 500U)) {
        btn2_long_press_start_tick = now;
        btn2_last_freq_step_tick   = now - BTN2_FREQ_STEP_INTERVAL_SLOW_MS;
    }

    last_hold_call_tick = now;
    hold_ms             = (uint32_t)(now - btn2_long_press_start_tick);

    if (hold_ms < BTN2_FREQ_ACCEL_STAGE1_MS) {
        interval_ms = BTN2_FREQ_STEP_INTERVAL_SLOW_MS;
    } else if (hold_ms < BTN2_FREQ_ACCEL_STAGE2_MS) {
        interval_ms = BTN2_FREQ_STEP_INTERVAL_MEDIUM_MS;
    } else if (hold_ms < BTN2_FREQ_ACCEL_STAGE3_MS) {
        interval_ms = BTN2_FREQ_STEP_INTERVAL_FAST_MS;
    } else {
        interval_ms = BTN2_FREQ_STEP_INTERVAL_FASTEST_MS;
    }

    if ((uint32_t)(now - btn2_last_freq_step_tick) >= interval_ms) {
        float freq = Oscillate_GetFreq();
        if (Oscillate_GetFreqStepDirection() == 1) {
            freq += freq_config->freq_scan_step;
        } else {
            freq -= freq_config->freq_scan_step;
        }
        if (freq < 0) {
            freq = 0;
        }
        Oscillate_SetFreq(freq);
        btn2_last_freq_step_tick = now;
    }
}

// Initialize buttons
void buttons_init(void) {
    button_init(&btn1, getButtonLevel, 1, 1);
    button_init(&btn2, getButtonLevel, 1, 2);

    // Attach event handlers for button 1
    button_attach(&btn1, BTN_SINGLE_CLICK, btn1_single_click_handler);
    button_attach(&btn1, BTN_DOUBLE_CLICK, btn1_double_click_handler);
    button_attach(&btn1, BTN_LONG_PRESS_START, btn1_long_press_start_handler);
    button_attach(&btn1, BTN_LONG_PRESS_HOLD, btn1_long_press_hold_handler);
    button_attach(&btn1, BTN_PRESS_REPEAT, btn1_press_repeat_handler);
    // Attach event handlers for button 2
    button_attach(&btn2, BTN_SINGLE_CLICK, btn2_single_click_handler);
    button_attach(&btn2, BTN_DOUBLE_CLICK, btn2_double_click_handler);
    button_attach(&btn2, BTN_LONG_PRESS_HOLD, btn2_long_press_hold_handler);

    // Start button processing
    button_start(&btn1);
    button_start(&btn2);
}

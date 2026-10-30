#include "oled.h"
#include <string.h>

extern float Oscillate_GetFreq(void);
extern float Oscillate_GetDutyRatio(void);
extern float ldc1101_getLHRSampleRate(LDC_HandleTypeDef *hldc);
extern uint8_t Oscillate_GetFreqStepDirection(void);

static volatile uint8_t oled_update_pending;

#define OLED_SCREEN_WIDTH 128U

static uint8_t OLED_FontCharWidth(uint8_t font_size) { return (font_size == 16U) ? 8U : 6U; }

static uint8_t OLED_MaxU8(uint8_t a, uint8_t b) { return (a > b) ? a : b; }

static uint8_t OLED_U32DigitCount(uint32_t value) {
    uint8_t digits = 1U;

    while (value >= 10U) {
        value /= 10U;
        digits++;
    }

    return digits;
}

static uint8_t OLED_FloatIntegerDigitCount(float value) {
    if (value < 0.0f) {
        value = -value;
    }

    return OLED_U32DigitCount((uint32_t)value);
}

static uint16_t OLED_AdvanceChars(uint16_t x, uint8_t font_size, uint8_t char_count) {
    return x + (uint16_t)OLED_FontCharWidth(font_size) * char_count;
}

static uint16_t OLED_StringWidth(const char *str, uint8_t font_size) {
    return (uint16_t)strlen(str) * OLED_FontCharWidth(font_size);
}

static uint16_t OLED_DecimalWidth(float value, uint8_t integer_digits, uint8_t decimal_digits, uint8_t font_size) {
    uint8_t char_count = integer_digits;

    if (value < 0.0f) {
        char_count++;
    }

    if (decimal_digits > 0U) {
        char_count += (uint8_t)(1U + decimal_digits); // 小数点 + 小数位
    }

    return (uint16_t)char_count * OLED_FontCharWidth(font_size);
}

static uint16_t OLED_ShowStringAdvance(uint16_t x, uint8_t y, const char *str, uint8_t font_size, uint8_t color_turn) {
    if (x < OLED_SCREEN_WIDTH) {
        OLED_ShowString((uint8_t)x, y, (char *)str, font_size, color_turn);
    }

    return x + OLED_StringWidth(str, font_size);
}

static uint16_t OLED_ShowNumAdvance(uint16_t x, uint8_t y, uint32_t value, uint8_t min_digits, uint8_t font_size,
                                    uint8_t color_turn) {
    uint8_t digits = OLED_MaxU8(OLED_U32DigitCount(value), min_digits);

    if (x < OLED_SCREEN_WIDTH) {
        OLED_ShowNum((uint8_t)x, y, (unsigned int)value, digits, font_size, color_turn);
    }

    return OLED_AdvanceChars(x, font_size, digits);
}

static uint16_t OLED_ShowDecimalAdvance(uint16_t x, uint8_t y, float value, uint8_t min_integer_digits,
                                        uint8_t decimal_digits, uint8_t font_size, uint8_t color_turn) {
    uint8_t integer_digits = OLED_MaxU8(OLED_FloatIntegerDigitCount(value), min_integer_digits);

    if (x < OLED_SCREEN_WIDTH) {
        OLED_Showdecimal((uint8_t)x, y, value, integer_digits, decimal_digits, font_size, color_turn);
    }

    return x + OLED_DecimalWidth(value, integer_digits, decimal_digits, font_size);
}

static uint16_t OLED_ShowText(uint16_t x, uint8_t y, const char *str) {
    return OLED_ShowStringAdvance(x, y, str, OLED_TEXT_SIZE, OLED_TEXT_INVERT);
}

static uint16_t OLED_ShowNumAuto(uint16_t x, uint8_t y, uint32_t value, uint8_t min_digits) {
    return OLED_ShowNumAdvance(x, y, value, min_digits, OLED_TEXT_SIZE, OLED_TEXT_INVERT);
}

static uint16_t OLED_ShowDecimalAuto(uint16_t x, uint8_t y, float value, uint8_t min_integer_digits,
                                     uint8_t decimal_digits) {
    return OLED_ShowDecimalAdvance(x, y, value, min_integer_digits, decimal_digits, OLED_TEXT_SIZE, OLED_TEXT_INVERT);
}

static const char *Mode_ToString(SysWorkMode mode) {
    switch (mode) {
        case MODE_MEASUREMENT:
            return "Measurement";
        case MODE_CALIBRATION:
            return "Calibration";
        case MODE_PULSE_FEEDBACK:
            return "Pulse Feedback";
        default:
            return "UNKNOWN";
    }
}

void OLED_Update(SysWorkMode system_mode) {
    float sample_rate = ldc1101_getLHRSampleRate(&hldc1);
    float pwm_freq    = Oscillate_GetFreq();
    float duty_ratio  = Oscillate_GetDutyRatio();
    uint16_t x;

    OLED_Clear();

    /* 第一行 */
    x = OLED_ShowText(0U, 0U, "Mode: ");
    OLED_ShowText(x, 0U, Mode_ToString(system_mode));

    /* 第二行 */
    x = OLED_ShowText(0U, 1U, "Sample Rate: ");
    OLED_ShowDecimalAuto(x, 1U, sample_rate, 2U, 2U);

    /* 第三行 */
    x = OLED_ShowText(0U, 2U, "PWM: ");
    x = OLED_ShowDecimalAuto(x, 2U, pwm_freq, 1U, 1U);
    x = OLED_ShowText(x, 2U, "/");
    OLED_ShowDecimalAuto(x, 2U, duty_ratio, 1U, 1U);
    OLED_ShowText(OLED_SCREEN_WIDTH - OLED_FontCharWidth(OLED_TEXT_SIZE), 2U,
                  Oscillate_GetFreqStepDirection() ? "+" : "-");

    /* 第四行 */
    x = OLED_ShowText(0U, 3U, "Freq Scan: ");
    x = OLED_ShowNumAuto(x, 3U, (uint32_t)freq_config->freq_scan_start, 2U);
    x = OLED_ShowText(x, 3U, "-");
    OLED_ShowNumAuto(x, 3U, (uint32_t)freq_config->freq_scan_end, 2U);

    /* 第 6、7 行：16 号字会占用两页 */
    OLED_ShowString(0U, 6U, "Viscosity: ", 16U, OLED_TEXT_INVERT);

    OLED_Refresh_DMA();
    OLED_ClearUpdatePending();
}

void OLED_SetUpdatePending(void) { oled_update_pending = true; }
void OLED_ClearUpdatePending(void) { oled_update_pending = false; }
uint8_t OLED_GetUpdatePending(void) { return oled_update_pending; }

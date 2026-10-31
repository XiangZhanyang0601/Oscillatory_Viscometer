#ifndef CONFIG_H
#define CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#include "drv8833_driver.h"
#include "ldc1101_driver.h"
#include "led.h"

/* =========================================================
 *                    System Work Mode
 * ========================================================= */
typedef enum {
    MODE_MEASUREMENT    = 0, // Mode0：测量模式
    MODE_CALIBRATION    = 1, // Mode1：静态位移标定
    MODE_PULSE_FEEDBACK = 2  // Mode2：脉冲响应测试
} SysWorkMode;

#define stable_time 1000U // 系统稳定时间(ms)，用于测量模式下的稳定性判断

// #define FREQ_SCAN_POINTS ((uint16_t)(((FREQ_SCAN_END_HZ - FREQ_SCAN_START_HZ) / FREQ_SCAN_STEP_HZ) + 0.5f) + 1)
// #define DR_SCAN_POINTS   ((uint16_t)(((DUTY_RATIO_SCAN_END - DUTY_RATIO_SCAN_START) / DUTY_RATIO_SCAN_STEP) + 0.5f) +
// 1)

/* =========================================================
 *               频率与占空比配置结构体
 * ========================================================= */
typedef struct {
    /* 频率配置 */
    float freq_default;         /* 默认频率，Hz */
    float freq_scan_start;      /* 扫频起始频率，Hz */
    float freq_scan_end;        /* 扫频结束频率，Hz */
    float freq_scan_step;       /* 扫频步进，Hz */
    uint32_t freq_scan_hold_ms; /* 每个频率保持时间，ms */

    /* 占空比配置 */
    float duty_default;         /* 默认占空比，% */
    float duty_scan_start;      /* 占空比扫描起点，% */
    float duty_scan_end;        /* 占空比扫描终点，% */
    float duty_scan_step;       /* 占空比扫描步进，百分点 */
    uint32_t duty_scan_hold_ms; /* 每个占空比保持时间，ms */
} FrequencyConfig;

extern const FrequencyConfig * const freq_config;

/* =========================================================
 *                  Calibration Configuration
 * ========================================================= */
#define CAL_SAMPLE_EPISODE   10U
#define CAL_SAMPLE_NUM       1000U
#define CALIBRATION_START_UM 450U
#define CALIBRATION_STEP_UM  1U
#define CALIBRATION_STEP_DIR 1U // 1: 减少，0：原地观测，-1：增加

/* =========================================================
 *                 Pulse Feedback Configuration
 * ========================================================= */
#define PERIOD_CNT_FOR_PULSE       200U // 振动周期计数阈值，达到后停止振动
#define PULSE_FEEDBACK_MAX_NUM     20000U
#define PULSE_FEEDBACK_TRIGGER_MAX 10U

/* =========================================================
 *     LDC1101 Hardware Registration and Configuration
 * ========================================================= */
extern const LDC_InitTypeDef LDC1_hardware;
extern const uint16_t LDC_LHR_cfg_size;
extern const LDC_RegConfig_t LDC_LHR_cfg[];

/* =========================================================
 *                DRV8833 Hardware Registration
 * ========================================================= */
extern const DRV_InitTypeDef DRV1_hardware;

/* =========================================================
 *                  Key Hardware Registration
 * ========================================================= */
#define KEY1_GPIO_PORT GPIOA
#define KEY1_PIN       GPIO_PIN_0
#define KEY2_GPIO_PORT GPIOD
#define KEY2_PIN       GPIO_PIN_1

/* =========================================================
 *                  LED Hardware Registration
 * ========================================================= */
extern const LED_InitTypeDef LED1_hardware;
extern const LED_InitTypeDef LED2_hardware;

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_H */

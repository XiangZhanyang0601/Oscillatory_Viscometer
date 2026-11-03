#include "config.h"
extern TIM_HandleTypeDef htim2;
extern SPI_HandleTypeDef hspi2;
/* =========================================================
 *              DRV8833 Hardware Source
 * ========================================================= */
const DRV_InitTypeDef DRV1_hardware = {
    .htim = &htim2, .CH1 = TIM_CHANNEL_1, .CH2 = TIM_CHANNEL_2, .nSLEEP_Port = GPIOA, .nSLEEP_Pin = GPIO_PIN_4};
/* =========================================================
 *     LDC1101 Hardware Source and Configuration
 * ========================================================= */
const LDC_InitTypeDef LDC1_hardware = {.hspi = &hspi2, .cs_port = GPIOB, .cs_pin = GPIO_PIN_12};
const LDC_RegConfig_t LDC_LHR_cfg[] = {
    // 基本参数设置，LHR 模式只关注 RP_MIN，需要与线圈参数对应
    {_LDC1101_REG_CFG_RP_MEASUREMENT_DYNAMIC_RANGE, _LDC1101_RP_SET_RP_MIN_1_5KOhm | _LDC1101_RP_SET_RP_MAX_3KOhm},
    // 进入 LHR 模式
    {_LDC1101_REG_CFG_ADDITIONAL_DEVICE, _LDC1101_ALT_CFG_L_OPTIMAL_ENABLE},
    {_LDC1101_REG_AMPLITUDE_CONTROL_REQUIREMENT, 0x01}, // 振幅不足也持续转换
    // 中断报告事件
    {_LDC1101_REG_CFG_INTB_MODE, _LDC1101_INTB_MODE_DONT_REPORT_INTB_ON_SDO_PIN},
    // 采样率
    {_LDC1101_REG_LHR_RCOUNT_LSB, 0x49},
    {_LDC1101_REG_LHR_RCOUNT_MSB, 0x01},
    // 偏移修正（初始值，后续可以动态调整）
    {_LDC1101_REG_LHR_OFFSET_LSB, 0x00},
    {_LDC1101_REG_LHR_OFFSET_MSB, 0x00},
    // 分频设置，减小线圈输入频率以保证 LDC 输入频率（16MHz）始终大于 4 倍的传感器频率
    {_LDC1101_REG_CFG_LHR, _LDC1101_LHR_CFG_FREQUENCY_NOT_DIVIDED},
};
const uint16_t LDC_LHR_cfg_size = sizeof(LDC_LHR_cfg) / sizeof(LDC_LHR_cfg[0]);

/* =========================================================
 *                     LED Hardware Source
 * ========================================================= */
const LED_InitTypeDef LED1_hardware = {.GPIOx = GPIOB, .GPIO_Pin = GPIO_PIN_2, .ActiveLevel = LED_ACTIVE_HIGH};
const LED_InitTypeDef LED2_hardware = {.GPIOx = GPIOD, .GPIO_Pin = GPIO_PIN_0, .ActiveLevel = LED_ACTIVE_HIGH};

/* =========================================================
 *             Viscometer Version Configuration
 * ========================================================= */
/* 版本表 */
typedef enum {
    VM_VERSION_1_0 = 0,

    VM_VERSION_2_0_L,
    VM_VERSION_2_0_M,
    VM_VERSION_2_0_H,

    VM_VERSION_2_1_L,
    VM_VERSION_2_1_M,

    VM_VERSION_2_2_L_6V,
    VM_VERSION_2_2_L_8V,
    VM_VERSION_2_2_L_10V,
    VM_VERSION_2_2_H,

    VM_VERSION_COUNT /* 版本数量 */
} FrequencyVersion;

/* 版本参数表*/
static const FrequencyConfig freq_config_table[VM_VERSION_COUNT] = {
    /*
     * 默认频率  扫频起点  扫频终点  频率步进  保持时间
     * 默认占空比  扫描起点  扫描终点  占空比步进  保持时间
     */
    {547.0f, 544.5f, 551.4f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U}, /* 1_0 */

    {272.0f, 0.0f, 0.0f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U},  /* 2_0_L */
    {547.0f, 0.0f, 0.0f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U},  /* 2_0_M */
    {1043.5f, 0.0f, 0.0f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U}, /* 2_0_H */

    {225.0f, 272.0f, 277.2f, 0.1f, 30U, 84.0f, 83.0f, 84.0f, 0.1f, 10U}, /* 2_1_L */
    {547.0f, 0.0f, 0.0f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U},     /* 2_1_M */

    {278.0f, 276.0f, 281.8f, 0.1f, 30U, 86.3f, 80.0f, 90.0f, 0.1f, 10U}, /* 2_2_L （6V-1120）*/
    {278.0f, 276.0f, 281.8f, 0.1f, 30U, 86.3f, 80.0f, 90.0f, 0.1f, 10U}, /* 2_2_L （8V-1120）*/
    {278.0f, 276.0f, 281.8f, 0.1f, 30U, 86.3f, 86.3f, 86.5f, 0.1f, 10U}, /* 2_2_L （10V-1120）*/
    {0.0f, 0.0f, 0.0f, 0.1f, 30U, 61.0f, 0.0f, 100.0f, 0.1f, 10U},       /* 2_2_H */
};

/* 当前版本选择 */
const FrequencyConfig * const freq_config = &freq_config_table[VM_VERSION_2_2_L_8V];
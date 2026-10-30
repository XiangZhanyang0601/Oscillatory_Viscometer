#include "oscillate.h"

extern DRV_HandleTypeDef hdrv1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim4;

extern SysWorkMode system_mode;

static Oscillate_State_t s_oscillate_state        = OSCILLATE_STOPPED;
static OscillateScanCallback_t s_scan_callback    = 0;
static volatile uint8_t s_oscillate_freq_increase = 1;

static volatile uint8_t drv_excitingLevel = 0;
static float drv_PWM_freq                 = 0.0f;
static float drv_PWM_DR                   = 0.0f;

static volatile uint32_t s_PWM_cnt       = 0;
static volatile uint32_t s_PWM_halfCnt   = 0;
static volatile uint32_t s_PWM_assertCnt = 0;
static Scan_Type_t s_scan_type           = SCAN_IDLE;
static uint32_t s_tim1_cnt               = 0;
static uint32_t s_tim4_cnt               = 0;

static uint8_t s_pulse_period_cnt = 0;

// 回调函数
void Oscillate_RegisterScanCallback(OscillateScanCallback_t cb) { s_scan_callback = cb; }
static void Oscillate_NotifyScanCallback(Oscillate_Scan_Event_t event) {
    if (s_scan_callback) {
        s_scan_callback(event);
    }
}

// 内部函数前向声明
static void Oscillate_CalculatePWMParam(void);

void Oscillate_Init(void) {
    DRV_Sleep(&hdrv1);
    s_oscillate_state = OSCILLATE_STOPPED;
    s_scan_type       = SCAN_IDLE;
    drv_PWM_freq      = freq_config->freq_default;
    drv_PWM_DR        = freq_config->duty_default;
    Oscillate_CalculatePWMParam();
}
void Oscillate_Start(void) {
    // 重置相位计数
    s_tim1_cnt        = 0;
    drv_excitingLevel = false;
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);

    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_PWM_Start(hdrv1.CHANNEL_A.htim, hdrv1.CHANNEL_A.CH1);
    HAL_TIM_PWM_Start(hdrv1.CHANNEL_A.htim, hdrv1.CHANNEL_A.CH2);
    DRV_Wake(&hdrv1);
    s_oscillate_state = OSCILLATE_RUNNING;
}
void Oscillate_Stop(void) {
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_TIM_PWM_Stop(hdrv1.CHANNEL_A.htim, hdrv1.CHANNEL_A.CH1);
    HAL_TIM_PWM_Stop(hdrv1.CHANNEL_A.htim, hdrv1.CHANNEL_A.CH2);
    DRV_Coast(&hdrv1);
    DRV_Sleep(&hdrv1);
    s_oscillate_state = OSCILLATE_STOPPED;
    if (s_scan_type != SCAN_IDLE) {
        s_scan_type = SCAN_IDLE;
        HAL_TIM_Base_Stop_IT(&htim4);
    }
}

void Oscillate_FreqStepDirectionToggle(void) { s_oscillate_freq_increase = !s_oscillate_freq_increase; }
uint8_t Oscillate_GetFreqStepDirection(void) { return s_oscillate_freq_increase; }

void Oscillate_ScanStart(Scan_Type_t type) {
    // 重置相位计数
    s_tim4_cnt = 0;
    __HAL_TIM_SET_COUNTER(&htim4, 0);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);

    s_scan_type = type;
    if (type == SCAN_FREQ) {
        drv_PWM_freq = (float)freq_config->freq_scan_start;
    } else if (type == SCAN_DR) {
        drv_PWM_DR = (float)freq_config->duty_scan_start;
    }
    Oscillate_CalculatePWMParam();
    HAL_TIM_Base_Start_IT(&htim4);

    // 通知应用层扫描开始
    Oscillate_NotifyScanCallback(OSC_SCAN_EVENT_START);
}
void Oscillate_ScanDone(void) {
    HAL_TIM_Base_Stop_IT(&htim4);
    s_scan_type = SCAN_IDLE;
    // 通知应用层扫描完成
    Oscillate_NotifyScanCallback(OSC_SCAN_EVENT_DONE);
}

static void Oscillate_CalculatePWMParam(void) {
    if (drv_PWM_freq <= 0.0f || drv_PWM_DR < 0.0f || drv_PWM_DR > 100.0f) {
        return;
    }
    // TIM1 Interrupt Frequency = 100kHz
    const float baseFreq = 100000.0f;
    // Calculate the number of interrupts needed for one period
    float cnt_f     = baseFreq / drv_PWM_freq;
    s_PWM_cnt       = (uint32_t)(cnt_f + 0.5f);
    float assert_f  = cnt_f * (drv_PWM_DR / 100.0f);
    s_PWM_assertCnt = (uint32_t)(assert_f + 0.5f);
    s_PWM_halfCnt   = s_PWM_cnt / 2;
}
void Oscillate_SetFreq(float freq) {
    if (freq <= 0.0f) {
        return;
    }
    drv_PWM_freq = freq;
    Oscillate_CalculatePWMParam();
}
void Oscillate_SetDutyRatio(float duty_ratio) {
    if ((duty_ratio < 0.0f) || (duty_ratio > 100.0f)) {
        return;
    }
    drv_PWM_DR = duty_ratio;
    Oscillate_CalculatePWMParam();
}

// 中断调用函数
void Oscillate_TickISR(void) {
    if (s_oscillate_state != OSCILLATE_RUNNING) {
        return;
    }
    if (s_tim1_cnt < s_PWM_assertCnt) {
        DRV_Forward(&hdrv1);
        drv_excitingLevel = true;
    } else if (s_tim1_cnt < s_PWM_halfCnt) {
        DRV_Coast(&hdrv1);
    } else if (s_tim1_cnt < s_PWM_halfCnt + s_PWM_assertCnt) {
        DRV_Reverse(&hdrv1);
        drv_excitingLevel = false;
    } else if (s_tim1_cnt < s_PWM_cnt) {
        DRV_Coast(&hdrv1);
    } else {
        s_tim1_cnt = 0;
        // TODO：任务变量在任务结束后没有初始化，会导致下次在脉冲模式下进入测量时出现异常。
        // 问题来源：代码层级问题，任务状态的变量错误放置在振动服务层进行控制判断。暂时没有时间改 2026.06.23
        if (system_mode == MODE_PULSE_FEEDBACK) {
            if (s_pulse_period_cnt >= PERIOD_CNT_FOR_PULSE) {
                Oscillate_Stop();
                return;
            } else
                s_pulse_period_cnt++;
        }
    }
    s_tim1_cnt++;
}
void Oscillate_ScanISR(void) {
    switch (s_scan_type) {
        case SCAN_FREQ:
            if (s_tim4_cnt >= freq_config->freq_scan_hold_ms) {
                s_tim4_cnt = 0;

                if (drv_PWM_freq + freq_config->freq_scan_step >= freq_config->freq_scan_end) {
                    Oscillate_ScanDone();
                } else {
                    drv_PWM_freq += freq_config->freq_scan_step;
                    Oscillate_CalculatePWMParam();
                }
            } else {
                s_tim4_cnt++;
            }
            break;

        case SCAN_DR:
            if (s_tim4_cnt >= freq_config->duty_scan_hold_ms) {
                s_tim4_cnt = 0;

                if (drv_PWM_DR + freq_config->duty_scan_step >= freq_config->duty_scan_end) {
                    Oscillate_ScanDone();
                } else {
                    drv_PWM_DR += freq_config->duty_scan_step;
                    Oscillate_CalculatePWMParam();
                }
            } else {
                s_tim4_cnt++;
            }
            break;

        default:
            break;
    }
}

// 参数获取函数
float Oscillate_GetFreq(void) { return drv_PWM_freq; }
float Oscillate_GetDutyRatio(void) { return drv_PWM_DR; }
bool Oscillate_GetExcitingLevel(void) { return drv_excitingLevel; }
Oscillate_State_t Oscillate_GetState(void) { return s_oscillate_state; }

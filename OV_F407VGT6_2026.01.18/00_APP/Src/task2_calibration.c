#include "task2_calibration.h"

#include "config.h"
#include "lhr_sample.h"
#include "pack.h"
#include "stm32f4xx_hal.h"
#include "usart3.h"
#include "LEDCtl.h"

#include <stddef.h>
#include <string.h>

#define TASK2_CALIBRATION_FRAME_SIZE 11U

typedef enum {
    CAL_STATE_IDLE = 0, /* 等待触发 */
    CAL_STATE_SAMPLING, /* 等待稳定并采样 */
    CAL_STATE_DONE      /* 采样结束，计算并发送结果 */
} CalibrationState_t;

/* ==================== Task2运行控制 ==================== */
static volatile bool s_task2_is_running;
static volatile bool s_task2_is_running_last;
static volatile bool s_task2_trigger;

/* ==================== 标定状态与位置 ==================== */
static CalibrationState_t s_cal_state;
static uint16_t s_cal_position_um;

/* ==================== 采样进度 ==================== */
static uint16_t s_cal_episode_index; /* 当前采样轮次：0～CAL_SAMPLE_EPISODE-1 */
static uint32_t s_cal_sample_cnt;    /* 当前轮次已采集的数据点数 */

/* ==================== 当前轮统计量 ==================== */
static double s_cal_episode_mean; // 当前轮次均值
static double s_cal_episode_m2;   // 当前轮次方差计算中间量 M2

/* ==================== 各轮统计结果 ==================== */
static double s_cal_episode_means[CAL_SAMPLE_EPISODE];
static double s_cal_episode_variances[CAL_SAMPLE_EPISODE];

/* ==================== 最终统计结果 ==================== */
static float s_cal_mean_result;
static float s_cal_variance_result;

void Task2_Calibration_Init(void) {
    s_task2_is_running      = false;
    s_task2_is_running_last = false;
    s_task2_trigger         = false;

    s_cal_state       = CAL_STATE_IDLE;
    s_cal_position_um = CALIBRATION_START_UM;

    s_cal_episode_index = 0U;
    s_cal_sample_cnt    = 0U;

    s_cal_episode_mean = 0.0;
    s_cal_episode_m2   = 0.0;

    memset(s_cal_episode_means, 0, sizeof(s_cal_episode_means));
    memset(s_cal_episode_variances, 0, sizeof(s_cal_episode_variances));

    s_cal_mean_result     = 0.0f;
    s_cal_variance_result = 0.0f;

    uint8_t frame[11];
    memset(frame, 0xFF, sizeof(frame));
    if (USART3_Enqueue(frame, sizeof(frame)))
        USART3_StartTrans();
}

void Task2_Calibration_Trigger(void) {
    if (!s_task2_trigger) {
        s_task2_trigger = true;
    }
}

void Task2_Calibration_Run(void) {
    uint32_t lhr_data;
    double sample;
    double delta;
    double mean_sum;
    double variance_sum;
    uint16_t i;
    uint8_t frame[TASK2_CALIBRATION_FRAME_SIZE];

    /*
     * 1. 消费触发请求。
     * Task2 运行期间产生的新触发直接清除，不累计到下一次流程。
     */
    if (s_task2_trigger) {
        s_task2_trigger = false;

        if (s_task2_is_running == false) {
            s_task2_is_running = true;
        }
    }

    /*
     * 2. 检测任务运行状态的变化。
     * 启动边沿负责清空本次标定的全部采样和统计变量。
     */
    if (s_task2_is_running != s_task2_is_running_last) {
        if (s_task2_is_running) {
            s_cal_episode_index = 0U;
            s_cal_sample_cnt    = 0U;
            s_cal_episode_mean  = 0.0;
            s_cal_episode_m2    = 0.0;

            memset(s_cal_episode_means, 0, sizeof(s_cal_episode_means));
            memset(s_cal_episode_variances, 0, sizeof(s_cal_episode_variances));

            s_cal_mean_result     = 0.0f;
            s_cal_variance_result = 0.0f;
            s_cal_state           = CAL_STATE_SAMPLING;

            LHRSample_Start();
        } else {
            LHRSample_Stop();
            s_cal_state = CAL_STATE_IDLE;
        }

        s_task2_is_running_last = s_task2_is_running;
    }

    if (s_task2_is_running == false) {
        return;
    }

    /*
     * 3. 非阻塞状态机。
     * 每次 Run() 最多处理一个由 LHRSample_Poll() 返回的新样本。
     */
    switch (s_cal_state) {
        case CAL_STATE_SAMPLING:
            if (LHRSample_Poll(&lhr_data) == false) {
                return;
            }

            /* 当前轮使用 Welford 算法在线计算均值和方差。 */
            sample = (double)lhr_data;
            s_cal_sample_cnt++;

            delta = sample - s_cal_episode_mean;
            s_cal_episode_mean += delta / (double)s_cal_sample_cnt;
            s_cal_episode_m2 += delta * (sample - s_cal_episode_mean);

            if (s_cal_sample_cnt < CAL_SAMPLE_NUM) {
                return;
            }

            /* 保存当前轮的均值和总体方差。 */
            s_cal_episode_means[s_cal_episode_index]     = s_cal_episode_mean;
            s_cal_episode_variances[s_cal_episode_index] = s_cal_episode_m2 / (double)s_cal_sample_cnt;

            s_cal_episode_index++;

            if (s_cal_episode_index >= CAL_SAMPLE_EPISODE) {
                s_cal_state = CAL_STATE_DONE;
                return;
            }

            /* 直接开始下一轮，不增加稳定等待。 */
            s_cal_sample_cnt   = 0U;
            s_cal_episode_mean = 0.0;
            s_cal_episode_m2   = 0.0;
            return;

        case CAL_STATE_DONE:
            mean_sum     = 0.0;
            variance_sum = 0.0;

            for (i = 0U; i < CAL_SAMPLE_EPISODE; i++) {
                mean_sum += s_cal_episode_means[i];
                variance_sum += s_cal_episode_variances[i];
            }

            s_cal_mean_result     = (float)(mean_sum / (double)CAL_SAMPLE_EPISODE);
            s_cal_variance_result = (float)(variance_sum / (double)CAL_SAMPLE_EPISODE);

            if (Pack_CalibrationFrame(s_cal_position_um, s_cal_mean_result, s_cal_variance_result, frame,
                                      (uint16_t)sizeof(frame))
                != PACK_OK) {
                s_task2_is_running = false;
                break;
            }

            /*
             * 队列暂满时保留 DONE 状态，下次 Run() 重新尝试，
             * 此时不能更新位置或结束本次任务。
             */
            if (USART3_Enqueue(frame, (uint16_t)sizeof(frame)) == false) {
                return;
            }

            USART3_StartTrans();

            if (CALIBRATION_STEP_DIR == 1) {
                if (s_cal_position_um >= CALIBRATION_STEP_UM) {
                    s_cal_position_um = (uint16_t)(s_cal_position_um - CALIBRATION_STEP_UM);
                } else {
                    s_cal_position_um = 0U;
                }
            } else if (CALIBRATION_STEP_DIR == -1) {
                if (s_cal_position_um <= (uint16_t)(UINT16_MAX - CALIBRATION_STEP_UM)) {
                    s_cal_position_um = (uint16_t)(s_cal_position_um + CALIBRATION_STEP_UM);
                } else {
                    s_cal_position_um = UINT16_MAX;
                }
            } else {
                /* CALIBRATION_STEP_DIR == 0：位置保持不变。 */
            }

            /*
             * 本次流程已经完整结束，立即同步运行状态，
             * 确保结束后第一次按键可以正常触发下一轮。
             */
            LHRSample_Stop();
            s_task2_is_running      = false;
            s_task2_is_running_last = false;
            s_cal_state             = CAL_STATE_IDLE;
            LEDCtl_On(LEDCTL_RUN);
            return;

        case CAL_STATE_IDLE:
        default:
            return;
    }
}
#include "task1_measurement.h"
#include "oscillate.h"
#include "lhr_sample.h"
#include "pack.h"
#include "usart3.h"

static volatile bool s_task1_is_running      = false;
static volatile bool s_task1_is_running_last = false;
static volatile uint32_t s_task1_hang_time   = 0;

static volatile bool s_scan_start_request = 0;
static volatile bool s_scan_done_request  = 0;

static void Task1_Measurement_ScanCallback(Oscillate_Scan_Event_t event);

void Task1_Measurement_Init(void) {
    if (Oscillate_GetState() == OSCILLATE_RUNNING) {
        Oscillate_Stop();
    }
    Oscillate_Init();
    LHRSample_Init();

    s_task1_is_running   = false;
    s_scan_start_request = 0;
    s_scan_done_request  = 0;

    // 回调函数注册
    Oscillate_RegisterScanCallback(Task1_Measurement_ScanCallback);
}

void Task1_Measurement_Start(void) { s_task1_is_running = true; }

void Task1_Measurement_Stop(void) { s_task1_is_running = false; }

void Task1_Measurement_Toggle(void) {
    if (s_task1_is_running == false) {
        s_task1_is_running = true;
    } else {
        s_task1_is_running = false;
    }
}

void Task1_Measurement_Run(void) {
    /*
     * 1. 扫描状态请求处理
     */
    if (s_scan_start_request) {
        s_scan_start_request = false;
        Task1_Measurement_Start();
    }
    if (s_scan_done_request) {
        s_scan_done_request = false;
        Task1_Measurement_Stop();
    }
    /*
     * 2. 任务状态边沿检测，控制振动与采样的启动与停止
     */
    if (s_task1_is_running != s_task1_is_running_last) {
        /* false -> true：启动 */
        if (s_task1_is_running == true) {
            LHRSample_Start();
            Oscillate_Start();
        }
        /* true -> false：停止 */
        else {
            Oscillate_Stop();
            LHRSample_Stop();
        }
        /* 状态更新 */
        s_task1_is_running_last = s_task1_is_running;
        if (s_task1_is_running == false) {
            return;
        }
    }
    /*
     * 3. 数据采集与打包发送
     */
    if (s_task1_is_running) {
        uint32_t lhr_data;
        uint8_t frame[12];

        if (LHRSample_Poll(&lhr_data) == false) {
            return;
        } else {
            Pack_MeasurementFrame(lhr_data, Oscillate_GetFreq(), Oscillate_GetDutyRatio(), Oscillate_GetExcitingLevel(),
                                  frame, sizeof(frame));
            if (USART3_Enqueue(frame, sizeof(frame))) {
                USART3_StartTrans();
            }
        }
    }
}

static void Task1_Measurement_ScanCallback(Oscillate_Scan_Event_t event) {
    switch (event) {
        // 扫描相关硬件由oscillate模块控制，应用层只负责开启振动和数据采样发送流程
        case OSC_SCAN_EVENT_START:
            s_scan_start_request = true;
            break;
        case OSC_SCAN_EVENT_DONE:
            s_scan_done_request = true;
            break;
        default:
            break;
    }
}
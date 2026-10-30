#include "command.h"
#include "ldc1101_driver.h"
#include "tim.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "task1_measurement.h"
#include "task2_calibration.h"

extern UART_HandleTypeDef huart2;

extern volatile uint8_t UART2_RX_activeBuffer;
extern char UART2_RX_DMA_buffer[2][MSG_LEN];
extern char UART2_TX_buffer[MSG_LEN];

extern LDC_HandleTypeDef hldc1;

void Command_Parse(char *cmd) {
    /* 设置占空比 */
    if (strncmp(cmd, "DR:", 3) == 0) {
        Oscillate_SetDutyRatio(strtof(cmd + 3, NULL));
        sprintf(UART2_TX_buffer, "Duty ratio set to %.2f%%\r\n", Oscillate_GetDutyRatio());
    }
    /* 设置频率 */
    else if (strncmp(cmd, "FR:", 3) == 0) {
        Oscillate_SetFreq(strtof(cmd + 3, NULL));
        sprintf(UART2_TX_buffer, "Frequency set to %.2f Hz\r\n", Oscillate_GetFreq());
    }
    /* 启动频率扫描 */
    else if (strncmp(cmd, "FR Scan", 7) == 0) {
        Oscillate_ScanStart(SCAN_FREQ);
        sprintf(UART2_TX_buffer, "Frequency Scan Start. \r\n");
    }
    /* 启动占空比扫描 */
    else if (strncmp(cmd, "DR Scan", 7) == 0) {
        Oscillate_ScanStart(SCAN_DR);
        sprintf(UART2_TX_buffer, "Duty Ratio Scan Start. \r\n");
    }
    /* 设置LDC采样频率 */
    else if (strncmp(cmd, "SampleRate:", 11) == 0) {
        if (ldc1101_setLHRSampleRate(&hldc1, strtof(cmd + 11, NULL))) {
            sprintf(UART2_TX_buffer, "LDC Sample Rate set to %.2f Hz (RCOUNT=0x%04X)\r\n",
                    ldc1101_getLHRSampleRate(&hldc1), ldc1101_getLHRRCount(&hldc1));
        } else {
            sprintf(UART2_TX_buffer, "Sample rate not valid or LDC not in sleep mode. Try again.\r\n");
        }
    }
    /* 读取LDC寄存器 */
    else if (strncmp(cmd, "Read:", 5) == 0) {
    }
    /* Task1 启动 */
    else if ((strncmp(cmd, "Task1 Toggle", 12) == 0)) {
        Task1_Measurement_Toggle();
        sprintf(UART2_TX_buffer, "Task1 toggled.\r\n");
    }
    /* Task2 启动 */
    else if ((strncmp(cmd, "Task2 Run", 9) == 0)) {
        Task2_Calibration_Trigger();
        sprintf(UART2_TX_buffer, "Task2 triggered.\r\n");
    } else {
        sprintf(UART2_TX_buffer, "Unknown command\r\n");
    }
    UART2_TX_send = true;
}

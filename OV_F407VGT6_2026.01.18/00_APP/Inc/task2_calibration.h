#ifndef TASK2_CALIBRITION_H
#define TASK2_CALIBRITION_H

#include "stdbool.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void Task2_Calibration_Init(void);
void Task2_Calibration_Trigger(void);
void Task2_Calibration_Run(void);

bool Task2_Calibration_SetPositionUm(uint16_t position_um);
uint16_t Task2_Calibration_GetPositionUm(void);

#ifdef __cplusplus
}
#endif
#endif /* TASK2_CALIBRATION_H */
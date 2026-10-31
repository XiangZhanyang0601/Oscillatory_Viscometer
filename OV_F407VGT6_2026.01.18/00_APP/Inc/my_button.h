#ifndef __MY_BUTTON_H
#define __MY_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "multi_button.h"

uint8_t getButtonLevel(uint8_t button_id);
void buttons_init(void);

/* 中断请求扫描，主循环查询并清除待扫描标志。 */
void Button_SetScanPending(void);
void Button_ClearScanPending(void);
bool Button_GetScanPending(void);

#ifdef __cplusplus
}
#endif

#endif /* __MY_BUTTON_H */

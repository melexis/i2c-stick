#ifndef __I2C_STICK_TASK_H__
#define __I2C_STICK_TASK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *handle_task(uint8_t channel_mask, char task);
void handle_task_next(uint8_t channel_mask);
void handle_task_help(uint8_t channel_mask);

#ifdef __cplusplus
}
#endif

#endif // __I2C_STICK_TASK_H__

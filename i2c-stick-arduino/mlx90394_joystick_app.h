#ifndef _MLX90394_JOYSTICK_CMD_
#define _MLX90394_JOYSTICK_CMD_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

uint8_t cmd_90394_joystick_app_begin(uint8_t channel_mask);
void handle_90394_joystick_app(uint8_t channel_mask);
uint8_t cmd_90394_joystick_app_end(uint8_t channel_mask);
void cmd_90394_joystick_ca(uint8_t channel_mask, const char *input);
void cmd_90394_joystick_ca_write(uint8_t channel_mask, const char *input);

#ifdef  __cplusplus
}
#endif

#endif

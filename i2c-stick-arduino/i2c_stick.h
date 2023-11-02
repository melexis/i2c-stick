#ifndef __I2C_STICK_H__
#define __I2C_STICK_H__

#include <stdint.h>
#include "i2c_stick_fw_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sa_drv_register_t
{
  uint16_t sa_: 7, drv_ : 6, disabled_ : 1, raw_: 1, nd_ : 1;
};

struct sa_list_t
{
  uint8_t spot_: 7, found_ : 1;
};


#define xstr(s) str(s)
#define str(s) #s


// HOST config
// 4 bit format field (nibble 0)
#define HOST_CFG_FORMAT_DEC  0
#define HOST_CFG_FORMAT_HEX  1
#define HOST_CFG_FORMAT_BIN  2
// 4 bit i2c field (nibble 1)
#define HOST_CFG_I2C_F100k   0
#define HOST_CFG_I2C_F400k   1
#define HOST_CFG_I2C_F1M     2
#define HOST_CFG_I2C_F50k    3
#define HOST_CFG_I2C_F20k    4
#define HOST_CFG_I2C_F10k    5


// application identifiers
#define APP_NONE              0

// mode demo operating mode (stored in g_mode)
#define MODE_INTERACTIVE 0
#define MODE_CONTINUOUS 1

// channel for external communication
#define CHANNEL_UART          0
#define CHANNEL_ETHERNET      1
#define CHANNEL_WIFI          2
#define CHANNEL_BLUETOOTH     3
#define CHANNEL_BUFFER        4


// global variables
extern int8_t g_mode;
extern uint16_t g_config_host;

extern sa_list_t g_sa_list[128];
extern sa_drv_register_t g_sa_drv_register[MAX_SA_DRV_REGISTRATIONS];
extern int8_t g_active_slave;

extern int8_t g_app_id;
extern int8_t g_state;


#ifdef BUFFER_COMMAND_ENABLE
  extern uint32_t g_channel_buffer_pos;
  extern char g_channel_buffer[BUFFER_CHANNEL_SIZE];
#endif // BUFFER_COMMAND_ENABLE


void send_broadcast_message(const char *msg);
void send_answer_chunk(uint8_t channel_mask, const char *answer, uint8_t terminate);
void send_answer_chunk_binary(uint8_t channel_mask, const char *blob, uint16_t length, uint8_t terminate);


int16_t i2c_stick_register_driver(uint8_t sa, uint8_t drv);

#ifdef __cplusplus
}
#endif

#endif // __I2C_STICK_H__

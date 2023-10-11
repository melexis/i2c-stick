#ifndef _MLX90614_CMD_
#define _MLX90614_CMD_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define MLX90614_LSB_C 50

struct MLX90614_t
{
  uint8_t slave_address_;
  unsigned long nd_timer_;
};

int16_t cmd_90614_register_driver();

void cmd_90614_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message);
void cmd_90614_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message);
void cmd_90614_nd(uint8_t sa, uint8_t *nd, char const **error_message);
void cmd_90614_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message);
void cmd_90614_cs(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90614_cs_write(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90614_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90614_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90614_is(uint8_t sa, uint8_t *is_ok, char const **error_message);

void cmd_90614_tear_down(uint8_t sa);

#ifdef  __cplusplus
}
#endif

#endif

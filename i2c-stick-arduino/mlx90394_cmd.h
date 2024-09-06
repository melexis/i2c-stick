#ifndef _MLX90394_CMD_
#define _MLX90394_CMD_

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct MLX90394_t
{
  uint8_t slave_address_;
  // local caching of sensor values whenever needed;
  // stored along with <SA>, such that multiple sensors can be supported.
  uint8_t enable_values_;
  uint8_t mode_;
};


int16_t cmd_90394_register_driver();

void cmd_90394_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message);
void cmd_90394_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message);
void cmd_90394_nd(uint8_t sa, uint8_t *nd, char const **error_message);
void cmd_90394_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message);
void cmd_90394_cs(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90394_cs_write(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90394_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90394_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90394_is(uint8_t sa, uint8_t *is_ok, char const **error_message);

void cmd_90394_tear_down(uint8_t sa);

#ifdef  __cplusplus
}
#endif

#endif

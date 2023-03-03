#ifndef _MLX90641_CMD_
#define _MLX90641_CMD_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mlx90641_api.h"

#define MLX90641_LSB_C 32

struct MLX90641_t
{
  uint8_t slave_address_;
  uint8_t flags_;
  float emissivity_;
  float t_room_;
  paramsMLX90641 mlx90641_;
  float *iir_;
};

// Flags for FIR stick operations. (These are not sensor settings)
#define MLX90641_CMD_FLAG_BROKEN_PIXELS         0
#define MLX90641_CMD_FLAG_IIR_FILTER            1

int16_t cmd_90641_register_driver();

MLX90641_t *cmd_90641_get_handle(uint8_t sa);
void cmd_90641_init(uint8_t sa);
void cmd_90641_tear_down(uint8_t sa);

void cmd_90641_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message);
void cmd_90641_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message);
void cmd_90641_nd(uint8_t sa, uint8_t *nd, char const **error_message);
void cmd_90641_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message);
void cmd_90641_cs(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90641_cs_write(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90641_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90641_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90641_is(uint8_t sa, uint8_t *is_ok, char const **error_message);


#ifdef __cplusplus
}
#endif

#endif

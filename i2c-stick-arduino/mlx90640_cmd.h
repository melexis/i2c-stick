#ifndef _MLX90640_CMD_
#define _MLX90640_CMD_

#include <stdint.h>
#include "mlx90640_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLX90640_LSB_C 32

struct MLX90640_t
{
  uint8_t slave_address_;
  uint8_t flags_;
  float emissivity_;
  float t_room_;
  float to_list_[768];
  float *iir_;
  paramsMLX90640 mlx90640_;
};

// Flags for FIR stick operations. (These are not sensor settings)
#define MLX90640_CMD_FLAG_BROKEN_PIXELS         0
#define MLX90640_CMD_FLAG_IIR_FILTER            1
#define MLX90640_CMD_FLAG_OUTLIER_PIXELS        2
#define MLX90640_CMD_FLAG_DEINTERLACE_FILTER    3
#define MLX90640_CMD_FLAG_ND_ON_FULL_FRAME      4

#define MLX90640_CMD_FLAG_IS_INIT               7

int16_t cmd_90640_register_driver();

MLX90640_t *cmd_90640_get_handle(uint8_t sa);
void cmd_90640_init(uint8_t sa);
void cmd_90640_tear_down(uint8_t sa);

void cmd_90640_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message);
void cmd_90640_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message);
void cmd_90640_nd(uint8_t sa, uint8_t *nd, char const **error_message);
void cmd_90640_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message);
void cmd_90640_cs(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90640_cs_write(uint8_t sa, uint8_t channel_mask, const char *input);
void cmd_90640_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90640_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message);
void cmd_90640_is(uint8_t sa, uint8_t *is_ok, char const **error_message);


#ifdef __cplusplus
}
#endif

#endif

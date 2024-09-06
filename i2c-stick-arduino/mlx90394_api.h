#ifndef __MLX90394_API__
#define __MLX90394_API__

//#ifdef  __cplusplus
//extern "C" {
//#endif

#include <stdint.h>

#define MLX90394_STAT1 0x00
#define MLX90394_STAT2 0x07
#define MLX90394_COMPANY_ID 0x0A
#define MLX90394_DEVICE_ID 0x0B
#define MLX90394_CTRL1 0x0E
#define MLX90394_CTRL2 0x0F
#define MLX90394_RESET 0x11
#define MLX90394_CTRL3 0x14
#define MLX90394_CTRL4 0x15

#define MLX90394_WOC_X 0x58
#define MLX90394_WOC_Y 0x5A
#define MLX90394_WOC_Z 0x5C

#define MLX90394_CTRL4_MASK_EN_T    (1U<<5)
#define MLX90394_CTRL1_MASK_EN_X    (1U<<4)
#define MLX90394_CTRL1_MASK_EN_Y    (1U<<5)
#define MLX90394_CTRL1_MASK_EN_Z    (1U<<6)

#define MLX90394_STAT1_MASK_DRDY    (1U<<0)


#define MLX90394_MODE_POWER_DOWN     0
#define MLX90394_MODE_SINGLE         1
#define MLX90394_MODE_5Hz            2
#define MLX90394_MODE_10Hz           3
#define MLX90394_MODE_15Hz           4
#define MLX90394_MODE_50Hz           5
#define MLX90394_MODE_100Hz          6
#define MLX90394_MODE_SELF_TEST      7
#define MLX90394_MODE_POWER_DOWN2    8
#define MLX90394_MODE_SINGLE2        9
#define MLX90394_MODE_200Hz         10
#define MLX90394_MODE_500Hz         11
#define MLX90394_MODE_700Hz         12
#define MLX90394_MODE_1000Hz        13
#define MLX90394_MODE_1400Hz        14
#define MLX90394_MODE_POWER_DOWN3   15


int mlx90394_init(uint8_t sa);
int mlx90394_write_measurement_mode(uint8_t sa, uint8_t mode);
int mlx90394_trigger_measurement(uint8_t sa);
int mlx90394_read_data_ready_bit(uint8_t sa);
int mlx90394_read_xyzt(uint8_t sa, int16_t *x, int16_t *y, int16_t *z, int16_t *t);
int mlx90394_measure_xyzt(uint8_t sa, int16_t *x, int16_t *y, int16_t *z, int16_t *t, int16_t timeout_us = -1);
int mlx90394_write_EN_X(uint8_t sa, uint8_t enable = 1);
int mlx90394_write_EN_Y(uint8_t sa, uint8_t enable = 1);
int mlx90394_write_EN_Z(uint8_t sa, uint8_t enable = 1);
int mlx90394_write_EN_T(uint8_t sa, uint8_t enable = 1);

//#ifdef  __cplusplus
//}
//#endif


#endif // __MLX90394_API__

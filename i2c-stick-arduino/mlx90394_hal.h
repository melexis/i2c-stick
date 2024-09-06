#ifndef __MLX90394_I2C_HAL__
#define __MLX90394_I2C_HAL__

#include <stdint.h>

//#ifdef  __cplusplus
//extern "C" {
//#endif

void mlx90394_i2c_init();
int mlx90394_i2c_direct_read(uint8_t sa, uint16_t *data, uint8_t count);
int mlx90394_i2c_addressed_read(uint8_t sa, uint8_t read_address, uint16_t *data, uint8_t count);
int mlx90394_i2c_addressed_write(uint8_t sa, uint8_t write_address, uint8_t data);
void mlx90394_i2c_set_clock_frequency(int freq);

void mlx90394_delay_us(int32_t delay_us);

//#ifdef  __cplusplus
//}
//#endif

#endif // __MLX90394_I2C_HAL__

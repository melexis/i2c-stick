/* 
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
#ifndef _MLX90632_LIBRARY_CALLS_
#define _MLX90632_LIBRARY_CALLS_

#include "mlx90632_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

struct Mlx90632Device;

int16_t _mlx90632_compute_library(struct Mlx90632Device *mlx, double *ta, double *to);


int16_t mlx90632_compute_library(double *ta, double *to);


void mlx90632_drv_set_I2C_slave_address(uint8_t slave_address);
uint8_t mlx90632_drv_get_I2C_slave_address();



#ifdef ESP_PLATFORM
#define HAS_USLEEP
#endif

#ifdef  __cplusplus
}
#endif

#endif /* _MLX90632_LIBRARY_CALLS_ */
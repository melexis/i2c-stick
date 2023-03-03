/* 
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
/**
 * @file mlx90632_library_calls.c
 * @brief MLX90632 driver with abstract i2c communication
 * @internal
 *
 * @copyright (C) 2020 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @endinternal
 *
 * @details
 *
 * @addtogroup mlx90632_private MLX90632 Internal library functions
 * @{
 *
 */

#include "mlx90632_api.h"
#include "mlx90632_hal.h"
#include "mlx90632_advanced.h"
#include "mlx90632.h"
#include "mlx90632_library_calls.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t g_slave_address = 58;


int16_t 
_mlx90632_compute_library(struct Mlx90632Device *mlx, double *ta, double *to)
{
  struct Mlx90632AdcData *adc_data = _mlx90632_get_adc_values(mlx);
  struct Mlx90632CalibData *calib_data = _mlx90632_get_calib_data(mlx);
  mlx90632_set_emissivity(_mlx90632_drv_get_emissivity(mlx));

  /* Get preprocessed temperatures needed for object temperature calculation */
  double pre_ambient = mlx90632_preprocess_temp_ambient(adc_data->RAM_6_, adc_data->RAM_9_,  calib_data->Gb_);

  int16_t object_new_raw = (adc_data->RAM_4_ + adc_data->RAM_5_) / 2;
  int16_t object_old_raw = (adc_data->RAM_7_ + adc_data->RAM_8_) / 2;

  double pre_object = mlx90632_preprocess_temp_object(
                        object_new_raw, object_old_raw,
                        adc_data->RAM_6_, adc_data->RAM_9_, calib_data->Ka_);


  /* Calculate object temperature */
  double object = mlx90632_calc_temp_object(pre_object, pre_ambient, calib_data->Ea_, calib_data->Eb_, calib_data->Ga_, calib_data->Fa_, calib_data->Fb_, (int16_t)calib_data->Ha_, (int16_t)calib_data->Hb_);

  float Ea = calib_data->Ea_;
  float Eb = calib_data->Eb_;
  Ea /= (1ULL << 16);
  Eb /= (1ULL <<  8);

  *ta = ((pre_ambient - Eb) / Ea) + 25.0f + 273.15f;
  *to = object + 273.15f;
  return 5;
}


void 
mlx90632_drv_set_I2C_slave_address (uint8_t slave_address)
{
  g_slave_address = slave_address;
}


uint8_t 
mlx90632_drv_get_I2C_slave_address()
{
  return g_slave_address;
}


int32_t 
mlx90632_i2c_read(int16_t register_address, uint16_t *value)
{
  return _mlx90632_i2c_read_block(g_slave_address, (uint16_t)(register_address), value, 1);
}


int32_t 
mlx90632_i2c_write(int16_t register_address, uint16_t value)
{
  return _mlx90632_i2c_write(g_slave_address, (uint16_t)(register_address), value);
}


#ifndef HAS_USLEEP

void
usleep(int min_range, int max_range)
{
  _usleep(min_range, max_range);
}

#endif

extern struct Mlx90632Device *g_mlx90632;

int16_t
mlx90632_compute_library(double *ta, double *to)
{
  return _mlx90632_compute_library(g_mlx90632, ta, to);
}

#ifdef __cplusplus
}
#endif

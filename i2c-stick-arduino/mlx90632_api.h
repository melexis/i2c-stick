/* 
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
/**
 * @file mlx90632_api.h
 * @brief MLX90632 driver with virtual i2c communication
 * @internal
 *
 * @copyright (C) 2017 Melexis N.V.
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
 * @addtogroup mlx90632_API MLX90632 Driver Library API
 * Implementation of MLX90632 driver with virtual i2c read/write functions
 *
 * @details
 * Copy of Kernel driver, except that it is stripped of Linux kernel specifics
 * which are replaced by simple i2c read/write functions. There are some Linux
 * kernel macros left behind as they make code more readable and easier to
 * understand, but if you already have your own implementation then preprocessor
 * should handle it just fine.
 *
 * Repository contains README.md for compilation and unit-test instructions.
 *
 * @{
 *
 */
#ifndef _MLX90632_API_LIB_
#define _MLX90632_API_LIB_

#ifdef  __cplusplus
extern "C" {
#endif


/* no stdint.h on Renesas compiler for R8C. */
#if defined(NC30) || defined(NC77) || defined(NC79)
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;
typedef signed char       int8_t;
typedef signed short      int16_t;
typedef signed long       int32_t;
typedef signed long long  int64_t;
#else /* NCxx */
#include <stdint.h>
#endif        /* NCxx */

#define MLX90632_DRIVER_VERSION "V0.8.0"

enum MLX90632_RefreshRate
{
  MLX90632_RR_0Hz5 = 0,
  MLX90632_RR_1Hz,
  MLX90632_RR_2Hz,
  MLX90632_RR_4Hz,
  MLX90632_RR_8Hz,
  MLX90632_RR_16Hz,
  MLX90632_RR_32Hz,
  MLX90632_RR_64Hz,

  MLX90632_RR_2s = MLX90632_RR_0Hz5,
  MLX90632_RR_1s = MLX90632_RR_1Hz,
  MLX90632_RR_500ms = MLX90632_RR_2Hz,
  MLX90632_RR_250ms = MLX90632_RR_4Hz,
  MLX90632_RR_125ms = MLX90632_RR_8Hz,
  MLX90632_RR_63ms = MLX90632_RR_16Hz,
  MLX90632_RR_32ms = MLX90632_RR_32Hz,
  MLX90632_RR_16ms = MLX90632_RR_64Hz,
};


enum MLX90632_Reg_Mode
{
  MLX90632_REG_MODE_HALT,
  MLX90632_REG_MODE_SLEEPING_STEP,
  MLX90632_REG_MODE_STEP,
  MLX90632_REG_MODE_CONTINIOUS,
};


struct Mlx90632Device;


/*
 multiple slave (MLX90632) API
 *****************************
 - basic operation
*/
int16_t  _mlx90632_initialize(struct Mlx90632Device *mlx, uint8_t i2c_slave_address);
int16_t  _mlx90632_measure_degk(struct Mlx90632Device *mlx, float *ta_degk, float *to_degk);
int16_t  _mlx90632_measure_degc(struct Mlx90632Device *mlx, float *ta_degc, float *to_degc);
int16_t  _mlx90632_measure_degf(struct Mlx90632Device *mlx, float *ta_degf, float *to_degf);

/* - controlled operations */
int16_t  _mlx90632_soft_reset(struct Mlx90632Device *mlx);
int16_t  _mlx90632_start_of_conversion(struct Mlx90632Device *mlx);
int16_t  _mlx90632_has_new_data(struct Mlx90632Device *mlx);
int16_t  _mlx90632_wait_for_new_data(struct Mlx90632Device *mlx);
int16_t  _mlx90632_start_of_burst(struct Mlx90632Device *mlx);
int16_t  _mlx90632_has_eoc(struct Mlx90632Device *mlx);
int16_t  _mlx90632_wait_for_eoc(struct Mlx90632Device *mlx);

/* - configuration routines */
int16_t  _mlx90632_ee_write_refresh_rate(struct Mlx90632Device *mlx, enum MLX90632_RefreshRate refresh_rate);
int16_t  _mlx90632_ee_read_refresh_rate(struct Mlx90632Device *mlx, enum MLX90632_RefreshRate *refresh_rate);
int16_t  _mlx90632_reg_write_mode(struct Mlx90632Device *mlx, enum MLX90632_Reg_Mode mode);
int16_t  _mlx90632_reg_read_mode(struct Mlx90632Device *mlx, enum MLX90632_Reg_Mode *mode);
void     _mlx90632_drv_set_emissivity(struct Mlx90632Device *mlx, float emissivity);
float    _mlx90632_drv_get_emissivity(struct Mlx90632Device *mlx);


/*
 single slave (MLX90632) API
 ***************************
 */
/* - basic operation /*/
int16_t  mlx90632_initialize(uint8_t i2c_slave_address);
int16_t  mlx90632_measure_degk(float *ta_degk, float *to_degk);
int16_t  mlx90632_measure_degc(float *ta_degc, float *to_degc);
int16_t  mlx90632_measure_degf(float *ta_degf, float *to_degf);

/* - controlled operations */
int16_t  mlx90632_soft_reset();
int16_t  mlx90632_start_of_conversion();
int16_t  mlx90632_has_new_data();
int16_t  mlx90632_wait_for_new_data();
int16_t  mlx90632_start_of_burst();
int16_t  mlx90632_has_eoc();
int16_t  mlx90632_wait_for_eoc();

/* - configuration routines */
int16_t  mlx90632_ee_write_refresh_rate(enum MLX90632_RefreshRate refresh_rate);
int16_t  mlx90632_ee_read_refresh_rate(enum MLX90632_RefreshRate *refresh_rate);
int16_t  mlx90632_reg_write_mode(enum MLX90632_Reg_Mode mode);
int16_t  mlx90632_reg_read_mode(enum MLX90632_Reg_Mode *mode);
void     mlx90632_drv_set_emissivity(float emissivity);
float    mlx90632_drv_get_emissivity();
/**
@}
*/
#ifdef  __cplusplus
}
#endif

#endif
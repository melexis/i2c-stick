/* 
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
/**
 * @file mlx90632_advanced.h
 * @brief MLX90632 driver with virtual i2c communication
 * @internal
 *
 * @copyright (C) 2019 Melexis N.V.
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
#ifndef _MLX90632_ADVANCED_
#define _MLX90632_ADVANCED_


#include "mlx90632_api.h"
#include "mlx90632_library_calls.h"

/* Including CRC calculation functions */
#include <errno.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* ------------------------------------- *\
   logging in terminal section (hub)
\* ------------------------------------- */

/* 
#define MLX_HUB_PRINT_ENABLE
*/

#ifndef MLX_HUB_PRINT_ENABLE
/* no print by default... */
#define MLX_HUB_PRINT(a)
#else

  /*
  Enable log-level for INFO & DEBUG.
  */
  #define ENABLE_DEBUG
  #define ENABLE_INFO

  /* Select printing method for debug messages */

  /* for PC */
  #if defined(_WIN32) || defined(__unix) || defined(__APPLE__)
    #include <stdio.h>
    #define MLX_HUB_PRINT(a) printf a
  #endif

  /* for arduino */
  #ifdef ARDUINO
    #include <stdarg.h>

    #define MLX_HUB_PRINT(a) serial_printf a

    void serial_printf(const char *args, ...);
  #endif /* ARDUINO */

  /* for nrf52 SDK */
  #if defined(NRF51) || defined(NRF52) || defined(NRF91)
    #include "uart_log.h"
    #define MLX_HUB_PRINT(a) uart_log a; uart_poll();
  #endif
#endif

#ifndef ENABLE_DEBUG
#define MLX_DEBUG(a)
#else
#define MLX_DEBUG(a) MLX_HUB_PRINT(a)
#endif

#ifndef ENABLE_INFO
#define MLX_INFO(a)
#else
#define MLX_INFO(a) MLX_HUB_PRINT(a)
#endif

/* ----------------------------------------- *\
   END logging in terminal section (hub)
\* ----------------------------------------- */

/* BITS_PER_LONG is only used by mlx90632-library */
#undef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * 8)

/* Solve errno not defined values */
#ifndef ETIMEDOUT
#define ETIMEDOUT 110 /**< From linux errno.h */
#endif

#ifndef EINVAL
#define EINVAL 22 /**< From linux errno.h */
#endif

#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 93 /**< From linux errno.h */
#endif

#ifndef BIT
#define BIT(x)(1UL << (x))
#endif

#ifndef GENMASK
#define GENMASK(h, l) \
    (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0])) /**< Return number of elements in array */
#endif

/* Memory sections addresses */
#define MLX90632_ADDR_RAM       0x4000 /**< Start address of ram */
#define MLX90632_ADDR_EEPROM    0x2480 /**< Start address of user eeprom */

/* EEPROM addresses - used at startup */
#define MLX90632_EE_CTRL    0x24d4 /**< Control register initial value */
#define MLX90632_EE_CONTROL MLX90632_EE_CTRL /**< More human readable for Control register */

#define MLX90632_EE_I2C_ADDRESS 0x24d5 /**< I2C address register initial value */
#define MLX90632_EE_VERSION 0x240b /**< EEPROM version reg - assumed 0x101 */

/* calibration parameters at EEPROM */
#define MLX90632_EE_Ea 0x2424 /**< Ea calibration constant register 32bit */
#define MLX90632_EE_Eb 0x2426 /**< Eb calibration constant register 32bit */
#define MLX90632_EE_Fa 0x2428 /**< Fa calibration constant register 32bit */
#define MLX90632_EE_Fb 0x242a /**< Fb calibration constant register 32bit */
#define MLX90632_EE_Ga 0x242c /**< Ga calibration constant register 32bit */
#define MLX90632_EE_Gb 0x242e /**< Gb calibration constant 16bit */
#define MLX90632_EE_Ka 0x242f /**< Ka calibration constant 16bit */
#define MLX90632_EE_Ha 0x2481 /**< Ha customer calibration value register 16bit */
#define MLX90632_EE_Hb 0x2482 /**< Hb customer calibration value register 16bit */


/* Register addresses - volatile */
#define MLX90632_REG_I2C_ADDR   0x3000 /**< Chip I2C address register */

/* Control register address - volatile */
#define MLX90632_REG_CONTROL   0x3001 /**< Control Register address */
#define   MLX90632_CONTROL_SOB BIT(11)
#define   MLX90632_CONTROL_SOC BIT(3)
#define   MLX90632_CONTROL_MODE GENMASK(2, 1) /**< Mode Mask */
/* Modes */
#define     MLX90632_MODE_HALT       (0U << 1) /**< mode halt(reserved for test only) */
#define     MLX90632_MODE_SLEEP_STEP (1U << 1) /**< mode sleep step*/
#define     MLX90632_MODE_STEP       (2U << 1) /**< mode step */
#define     MLX90632_MODE_CONTINUOUS (3U << 1) /**< mode continuous*/

#define MLX90632_REG_I2C_CMD   0x3005 /**< I2C CMD Register address */


/* Device status register - volatile */
#define MLX90632_REG_STATUS 0x3fff /**< Device status register */
#define   MLX90632_STATUS_DEVICE_BUSY    BIT(10) /**< Device busy indicator */
#define   MLX90632_STATUS_EEPROM_BUSY    BIT(9) /**< Device EEPROM busy indicator */
#define   MLX90632_STATUS_BROWN_OUT      BIT(8) /**< Device brown out reset indicator */
#define   MLX90632_STATUS_CYCLE_POSITION GENMASK(6, 2) /**< Data position in measurement table */
#define   MLX90632_STATUS_NEW_DATA       BIT(0) /**< New Data indicator */
#define   MLX90632_STATUS_EOC            BIT(1) /**< End Of Conversion indicator (end of table) */

/* Magic constants */
#define MLX90632_EEPROM_WRITE_KEY 0x554C /**< EEPROM write key 0x55 and 0x4c */

struct Mlx90632CalibData
{
  int32_t Ea_;  
  int32_t Eb_;  
  int32_t Fa_;  
  int32_t Fb_;  
  int32_t Ga_;  
  uint16_t Gb_;  
  uint16_t Ka_;  
  uint16_t Ha_;
  int16_t Hb_;

  uint16_t emissivity_;

  uint16_t VDDNOM_OFFSET_;  
  uint16_t eeprom_version_;
};


struct Mlx90632AdcData
{
  int16_t RAM_4_;
  int16_t RAM_5_;
  int16_t RAM_6_;
  int16_t RAM_7_;
  int16_t RAM_8_;
  int16_t RAM_9_;
};



struct Mlx90632Device
{
  struct Mlx90632AdcData adc_data_;
  struct Mlx90632CalibData calib_data_;
  uint8_t slave_address_;

};


int32_t _mlx90632_i2c_read(uint8_t slave_address, uint16_t register_address, uint16_t *value);
int32_t _mlx90632_i2c_read_int32_t(uint8_t slave_address, uint16_t register_address, int32_t *value);



int16_t _mlx90632_read_adc(struct Mlx90632Device *mlx);
int16_t _mlx90632_ee_write(struct Mlx90632Device *mlx, uint16_t register_address, uint16_t new_value);
int16_t _mlx90632_read_calib_parameters(struct Mlx90632Device *mlx);
struct Mlx90632AdcData *_mlx90632_get_adc_values(struct Mlx90632Device *mlx);
struct Mlx90632CalibData *_mlx90632_get_calib_data(struct Mlx90632Device *mlx);

void     _mlx90632_print_calib_data (struct Mlx90632Device *mlx);
void     _mlx90632_print_adc_data (struct Mlx90632Device *mlx);



int16_t mlx90632_read_adc();
int16_t mlx90632_ee_write(uint16_t register_address, uint16_t new_value);
int16_t mlx90632_read_calib_parameters();
struct Mlx90632AdcData *mlx90632_get_adc_values();
struct Mlx90632CalibData *mlx90632_get_calib_data();
void     mlx90632_print_calib_data ();
void     mlx90632_print_adc_data ();

/**
@}
*/

#ifdef  __cplusplus
}
#endif

#endif

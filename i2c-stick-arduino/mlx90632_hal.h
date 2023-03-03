/* 
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
/**
 * @file mlx90632_hal.h
 * @brief MLX90632 driver Hardware Abstraction Layer (HAL)
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
 * @addtogroup mlx90632_hal MLX90632 Driver Library Externally defined
 * @brief Functions to be defined externally as they are uC or OS dependent
 *
 * @details
 * Some functions need to be declared externally as they are different for each
 * uC or OS. Depends on platform implementation SMBus functions read 8 bytes,
 * but MLX90632 has 16bit address registers (some are also 32bit). So reading
 * at least 16bits at time is recommended.
 *
 * @{
 */
#ifndef _MLX90632_HAL_LIB_
#define _MLX90632_HAL_LIB_

#include "mlx90632_api.h"

#ifdef  __cplusplus
extern "C" {
#endif


/** Read the register_address value from the mlx90632
 *
 * i2c read is processor specific and this function expects to have address of mlx90632 known, as it operates purely on
 * register addresses.
 *
 * @note Needs to be implemented externally
 * @param[in] register_address Address of the register to be read from
 * @param[out] *value pointer to where read data can be written

 * @retval 0 for success
 * @retval <0 for failure
 */


extern int32_t _mlx90632_i2c_read_block(uint8_t slave_address, uint16_t register_address, uint16_t *value, uint16_t size);



/** Write value to register_address of the mlx90632
 *
 * i2c write is processor specific and this function expects to have address of mlx90632 known, as it operates purely
 * on register address and value to be written to.
 *
 * @note Needs to be implemented externally
 * @param[in] register_address Address of the register to be read from
 * @param[in] value value to be written to register address of mlx90632

 * @retval 0 for success
 * @retval <0 for failure
 */
extern int32_t _mlx90632_i2c_write(uint8_t slave_address, uint16_t register_address, uint16_t value);

/** Blocking function for sleeping in microseconds
 *
 * Range of microseconds which are allowed for the thread to sleep. This is to avoid constant pinging of sensor if the
 * data is ready.
 *
 * @note Needs to be implemented externally
 * @param[in] min_range Minimum amount of microseconds to sleep
 * @param[in] max_range Maximum amount of microseconds to sleep
 */
extern void _usleep(int min_range, int max_range);

/**
@}
*/

#ifdef  __cplusplus
}
#endif

#endif
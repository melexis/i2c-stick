/*
   Driver: mlx90632-driver-library
   Version: V0.8.0
   Date: 16 June 2021
*/
/** 
 * @file mlx90632_core.c
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

#include "mlx90632_advanced.h"
#include "mlx90632_hal.h"
#include "mlx90632_library_calls.h"

#include <string.h>


/* gobal memory for a single MLX90632 */

struct Mlx90632Device g_mlx90632;

/* END gobal memory for a single MLX90632 */





int16_t
mlx90632_initialize(uint8_t i2c_slave_address)
{ 
  return _mlx90632_initialize(&g_mlx90632, i2c_slave_address); 
}


int16_t
mlx90632_measure_degk(float *ta_degk, float *to_degk)
{ 
  return _mlx90632_measure_degk(&g_mlx90632, ta_degk, to_degk); 
}


int16_t
mlx90632_measure_degc(float *ta_degc, float *to_degc)
{ 
  return _mlx90632_measure_degc(&g_mlx90632, ta_degc, to_degc); 
}


int16_t
mlx90632_measure_degf(float *ta_degf, float *to_degf)
{ 
  return _mlx90632_measure_degf(&g_mlx90632, ta_degf, to_degf); 
}



/* - controlled operations */
int16_t
mlx90632_soft_reset()
{
  return _mlx90632_soft_reset(&g_mlx90632);
}


int16_t
mlx90632_start_of_conversion()
{ 
  return _mlx90632_start_of_conversion(&g_mlx90632); 
}


int16_t
mlx90632_has_new_data()
{ 
  return _mlx90632_has_new_data(&g_mlx90632); 
}


int16_t
mlx90632_wait_for_new_data()
{ 
  return _mlx90632_wait_for_new_data(&g_mlx90632); 
}


int16_t
mlx90632_start_of_burst()
{
  return _mlx90632_start_of_burst(&g_mlx90632);
}


int16_t
mlx90632_has_eoc()
{
  return _mlx90632_has_eoc(&g_mlx90632);
}


int16_t
mlx90632_wait_for_eoc()
{
  return _mlx90632_wait_for_eoc(&g_mlx90632);
}


/* - configuration routines */
int16_t
mlx90632_ee_write(uint16_t register_address, uint16_t new_value)
{
  return _mlx90632_ee_write(&g_mlx90632, register_address, new_value);
}


int16_t
mlx90632_ee_write_refresh_rate(enum MLX90632_RefreshRate refresh_rate)
{
  return _mlx90632_ee_write_refresh_rate(&g_mlx90632, refresh_rate);
}


int16_t
mlx90632_ee_read_refresh_rate(enum MLX90632_RefreshRate *refresh_rate)
{
  return _mlx90632_ee_read_refresh_rate(&g_mlx90632, refresh_rate);
}


int16_t
mlx90632_reg_write_mode(enum MLX90632_Reg_Mode mode)
{
  return _mlx90632_reg_write_mode(&g_mlx90632, mode);
}


int16_t
mlx90632_reg_read_mode(enum MLX90632_Reg_Mode *mode)
{
  return _mlx90632_reg_read_mode(&g_mlx90632, mode);
}


void
mlx90632_drv_set_emissivity(float emissivity)
{
  _mlx90632_drv_set_emissivity(&g_mlx90632, emissivity);
}


float
mlx90632_drv_get_emissivity()
{
  return _mlx90632_drv_get_emissivity(&g_mlx90632);
}

int16_t
mlx90632_read_adc()
{
  return _mlx90632_read_adc(&g_mlx90632);
}


int16_t
mlx90632_read_calib_parameters()
{
  return _mlx90632_read_calib_parameters(&g_mlx90632);
}


struct
Mlx90632AdcData *mlx90632_get_adc_values()
{
  return _mlx90632_get_adc_values(&g_mlx90632);
}


struct
Mlx90632CalibData *mlx90632_get_calib_data()
{
  return _mlx90632_get_calib_data(&g_mlx90632);
}


void
mlx90632_print_calib_data()
{
  _mlx90632_print_calib_data(&g_mlx90632);
}


void
mlx90632_print_adc_data()
{
  _mlx90632_print_adc_data(&g_mlx90632);
}



struct Mlx90632AdcData *
_mlx90632_get_adc_values(struct Mlx90632Device *mlx)
{
  return &mlx->adc_data_;
}


struct Mlx90632CalibData *
_mlx90632_get_calib_data(struct Mlx90632Device *mlx)
{
  return &mlx->calib_data_;
}


int32_t
_mlx90632_i2c_read(uint8_t slave_address, uint16_t register_address, uint16_t *value)
{
  return _mlx90632_i2c_read_block(slave_address, register_address, value, 1);
}


int32_t
_mlx90632_i2c_read_int32_t(uint8_t slave_address, uint16_t register_address, int32_t *value)
{
  return _mlx90632_i2c_read_block(slave_address, register_address, (uint16_t *)value, 2);
}


/* generic API implementation */

int16_t 
_mlx90632_initialize(struct Mlx90632Device *mlx, uint8_t i2c_slave_address)
{
  mlx->slave_address_ = i2c_slave_address;
  memset(&mlx->adc_data_, 0, sizeof(mlx->adc_data_));
  mlx->meas_select_ = 0;
  mlx->meas_count_ = 3;

  if (_mlx90632_soft_reset(mlx) < 0)
  {
    return -2;
  }
  if (_mlx90632_read_calib_parameters(mlx))
  {
    return -1;
  }

  /* reset bit NEW_DATA and EOC */
  {
    uint16_t reg_status;
    if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_STATUS, &reg_status) < 0) return -3;
    reg_status &= ~(MLX90632_STATUS_NEW_DATA | MLX90632_STATUS_EOC);
    if (_mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_STATUS, reg_status) < 0) return -4;
  }
  return 0;
}


int16_t 
_mlx90632_read_adc(struct Mlx90632Device *mlx)
{
  const int TABLE_INDEX = 1;

  uint16_t *pointer = (uint16_t *)&mlx->adc_data_;

  if (_mlx90632_i2c_read_block(mlx->slave_address_, MLX90632_ADDR_RAM + 3 * TABLE_INDEX, pointer, 6)) return -1;

  /* reset bit NEW_DATA and EOC */
  {
    uint16_t reg_status;
    if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_STATUS, &reg_status) < 0) return -2;
    reg_status &= ~(MLX90632_STATUS_NEW_DATA | MLX90632_STATUS_EOC);
    if (_mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_STATUS, reg_status) < 0) return -3;
  }
  return 0;
}


int16_t
_mlx90632_ee_write(struct Mlx90632Device *mlx, uint16_t register_address, uint16_t new_value)
{ /* this kind of a smart EE write;
     1. first it reads the current value; if no change ==> done!
     2. if old data is not yet zero ==> erase
     3. when new data is different from zero => write!
     Warning ==> This routine performs a soft reset!
  */
  uint16_t old_value;
  int16_t ret = 0;
  /* 1. first it reads the current value; if no change ==> done! */
  if (_mlx90632_i2c_read(mlx->slave_address_, register_address, &old_value) < 0) return -1;
  if (old_value == new_value) return 0;
  /* 2. if old data is not yet zero ==> erase */
  _mlx90632_reg_write_mode(mlx, MLX90632_REG_MODE_HALT);
  if (old_value != 0)
  {
    if (!ret && _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_I2C_CMD, 0x554C) < 0) ret = -2;
    if (!ret && _mlx90632_i2c_write(mlx->slave_address_, register_address, 0) < 0) ret = -3;
  }
  /* 3. when new data is different from zero => write! */
  if (new_value != 0)
  {
    if (!ret && _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_I2C_CMD, 0x554C) < 0) ret = -12;
    if (!ret && _mlx90632_i2c_write(mlx->slave_address_, register_address, new_value) < 0) ret = -13;
  }
  _mlx90632_soft_reset(mlx);

  if (_mlx90632_i2c_read(mlx->slave_address_, register_address, &old_value) < 0) ret = -20;
  if (old_value != new_value) ret = -21;

  return ret;
}


int16_t 
_mlx90632_ee_write_refresh_rate(struct Mlx90632Device *mlx, enum MLX90632_RefreshRate refresh_rate)
{ /* write refresh rate code in EEPROM and perform a soft-reset */
  uint16_t reg_value;

  /* measurement slot #1 */
  if (_mlx90632_i2c_read(mlx->slave_address_, 0x24E1, &reg_value) < 0) return -1;
  reg_value &= ~(0x0007 << 8); /* set zero bits in refresh field */
  reg_value |= ((uint8_t)(refresh_rate) << 8); /* set one bits in refresh field */

  if (_mlx90632_ee_write(mlx, 0x24E1, reg_value) < 0) return -2;

  /* measurement slot #2 */
  if (_mlx90632_i2c_read(mlx->slave_address_, 0x24E2, &reg_value) < 0) return -11;
  reg_value &= ~(0x0007 << 8); /* set zero bits in refresh field */
  reg_value |= ((uint8_t)(refresh_rate) << 8); /* set one bits in refresh field */

  if (_mlx90632_ee_write(mlx, 0x24E2, reg_value) < 0) return -12;

  return 0;
}


int16_t 
_mlx90632_ee_read_refresh_rate(struct Mlx90632Device *mlx, enum MLX90632_RefreshRate *refresh_rate)
{
  uint16_t reg_value;
  if (_mlx90632_i2c_read(mlx->slave_address_, 0x24E1, &reg_value) < 0) return -1;
  *refresh_rate = (enum MLX90632_RefreshRate)((reg_value >> 8) & 0x0007);

  if (_mlx90632_i2c_read(mlx->slave_address_, 0x24E2, &reg_value) < 0) return -2;
  reg_value = (reg_value >> 8) & 0x0007;
  if (reg_value != *refresh_rate)
  {
    return -3; /* refresh rate in both measurement slots is not the same! */
  }

  return 0;
}


int16_t
_mlx90632_reg_write_mode(struct Mlx90632Device *mlx, enum MLX90632_Reg_Mode mode)
{
  uint16_t reg_control;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_CONTROL, &reg_control);
  if (ret < 0) return ret;

  reg_control &= ~MLX90632_CONTROL_MODE; /* Set mode bits to zero */
  reg_control |= ((mode << 1) & MLX90632_CONTROL_MODE); /* Set needed mode bits to one */

  ret = _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_CONTROL, reg_control);
  if (ret < 0) return ret;

  return 0;
}


int16_t 
_mlx90632_reg_read_mode(struct Mlx90632Device *mlx, enum MLX90632_Reg_Mode *mode)
{
  uint16_t reg_control;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_CONTROL, &reg_control);
  if (ret < 0) return ret;

  *mode = MLX90632_Reg_Mode((reg_control & MLX90632_CONTROL_MODE) >> 1);
  return ret;
}


void 
_mlx90632_drv_set_emissivity(struct Mlx90632Device *mlx, float emissivity)
{
  mlx->calib_data_.emissivity_ = (uint16_t)((emissivity * (1<<15)) + 0.5f);
}


float
_mlx90632_drv_get_emissivity(struct Mlx90632Device *mlx)
{
  return (float)(mlx->calib_data_.emissivity_) / (1<<15);
}

/* END generic API implementation */

/* Advanced API implementation */

int16_t 
_mlx90632_read_calib_parameters(struct Mlx90632Device *mlx)
{
  memset (&mlx->calib_data_, 0, sizeof (struct Mlx90632CalibData));

  if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_EE_VERSION, &mlx->calib_data_.eeprom_version_) < 0) return -1;

  mlx->calib_data_.emissivity_ = (1<<15); /* equivalent to 1.0 */
  if (_mlx90632_i2c_read_int32_t(mlx->slave_address_, MLX90632_EE_Ea, &mlx->calib_data_.Ea_) < 0) return -1;
  if (_mlx90632_i2c_read_int32_t(mlx->slave_address_, MLX90632_EE_Eb, &mlx->calib_data_.Eb_) < 0) return -1;
  if (_mlx90632_i2c_read_int32_t(mlx->slave_address_, MLX90632_EE_Fa, &mlx->calib_data_.Fa_) < 0) return -1;
  if (_mlx90632_i2c_read_int32_t(mlx->slave_address_, MLX90632_EE_Fb, &mlx->calib_data_.Fb_) < 0) return -1;
  if (_mlx90632_i2c_read_int32_t(mlx->slave_address_, MLX90632_EE_Ga, &mlx->calib_data_.Ga_) < 0) return -1;
  if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_EE_Gb, &mlx->calib_data_.Gb_) < 0) return -1;
  if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_EE_Ka, &mlx->calib_data_.Ka_) < 0) return -1;
  if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_EE_Ha, &mlx->calib_data_.Ha_) < 0) return -1;
  uint16_t data = 0;
  if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_EE_Hb, &data) < 0) return -1;
  mlx->calib_data_.Hb_ = (int16_t)(data);
  return 0;
}


int16_t
_mlx90632_soft_reset(struct Mlx90632Device *mlx)
{
  memset(&mlx->adc_data_, 0, sizeof(mlx->adc_data_));
  return _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_I2C_CMD, 0x0006);
}


int16_t 
_mlx90632_start_of_conversion(struct Mlx90632Device *mlx)
{ /* set SOC bit in CONTROL register */
  uint16_t reg_control;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_CONTROL, &reg_control);
  if (ret < 0) return ret;

  ret = _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_CONTROL, reg_control | MLX90632_CONTROL_SOC);
  if (ret < 0) return ret;

  return 0;
}


int16_t 
_mlx90632_has_new_data(struct Mlx90632Device *mlx)
{
  uint16_t reg_status;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_STATUS, &reg_status);
  if (ret < 0) return ret;

  if (reg_status & MLX90632_STATUS_NEW_DATA)
  {
    return 1;
  }

  return 0;
}


int16_t 
_mlx90632_wait_for_new_data(struct Mlx90632Device *mlx)
{
  uint16_t retries_allowed = 1100;
  for (; retries_allowed > 0; retries_allowed--)
  {
    int16_t has_new_data = _mlx90632_has_new_data(mlx);
    if (has_new_data < 0) return -1; /* return with error */
    if (has_new_data > 0) return 0; /* return with good result (with new data) */
    _usleep (4000, 4000);
  }
  return -2; /* timeout */
}


int16_t
_mlx90632_start_of_burst(struct Mlx90632Device *mlx)
{ /* set SOB bit in CONTROL register */
  uint16_t reg_control;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_CONTROL, &reg_control);
  if (ret < 0) return ret;

  ret = _mlx90632_i2c_write(mlx->slave_address_, MLX90632_REG_CONTROL, reg_control | MLX90632_CONTROL_SOB);
  if (ret < 0) return ret;

  return 0;
}


int16_t
_mlx90632_has_eoc(struct Mlx90632Device *mlx)
{
  uint16_t reg_status;

  int16_t ret = _mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_STATUS, &reg_status);

  if (ret < 0) return ret;

  if (reg_status & MLX90632_STATUS_EOC)
  {
    return 1;
  }

  return 0;
}


int16_t
_mlx90632_wait_for_eoc(struct Mlx90632Device *mlx)
{
  uint8_t retries_allowed = 200;
  for (; retries_allowed > 0; retries_allowed--)
  {
    int16_t has_eoc = _mlx90632_has_eoc(mlx);
    if (has_eoc < 0) return -1; /* return with error */
    if (has_eoc > 0) return 0; /* return with good result (with new data) */
    _usleep (10000, 11000);
  }
  return -2; /* timeout */
}


int16_t
_mlx90632_measure_degk(struct Mlx90632Device *mlx, float *ta_degk, float *to_degk)
{
  if (_mlx90632_wait_for_new_data(mlx)) return -3; /* no new data; timeout;... */
  if (mlx->adc_data_.RAM_6_ == 0) /* first time */
  { /* wait until full measurement cycle is completed (cycle_pos == 2) */
    uint16_t retries_allowed = 550;
    while (retries_allowed--)
    {
      uint16_t reg_status, cycle_pos;
      if (_mlx90632_i2c_read(mlx->slave_address_, MLX90632_REG_STATUS, &reg_status) < 0) return -2;
      cycle_pos = (reg_status & MLX90632_STATUS_CYCLE_POSITION) >> 2;
      if (cycle_pos == 2)
      {
        break;
      }
      _usleep(4000, 4000);
    }
  }
  if (_mlx90632_read_adc(mlx)) return -1; /* failed to read adc data... */

  {
    double ta = 0.0, to = 0.0;
    int16_t r = _mlx90632_compute_library(mlx, &ta, &to);
    *ta_degk = (float)(ta);
    *to_degk = (float)(to);

    if (r > 0) return 0; /* success */
    return 1; /* failure */
  }

  return 2; /* failure */
}


int16_t
_mlx90632_measure_degc(struct Mlx90632Device *mlx, float *ta_degc, float *to_degc)
{
  int16_t r = _mlx90632_measure_degk(mlx, ta_degc, to_degc);
  *ta_degc -= 273.15f;
  *to_degc -= 273.15f;
  return r;
}


int16_t
_mlx90632_measure_degf(struct Mlx90632Device *mlx, float *ta_degf, float *to_degf)
{
  /* (10 degC + 9/5) + 32 = 50 degF */
  int16_t r = _mlx90632_measure_degc(mlx, ta_degf, to_degf);
  *ta_degf *= 9;
  *ta_degf /= 5;
  *ta_degf += 32;
  *to_degf *= 9;
  *to_degf /= 5;
  *to_degf += 32;
  return r;
}


/* print functions.... */

void
_mlx90632_print_calib_data (struct Mlx90632Device *mlx)
{
  MLX_INFO(("mlx->calib_data_.eeprom_version_  = 0x%04X\n", mlx->calib_data_.eeprom_version_));

  MLX_INFO(("mlx->calib_data_.Ea_ = %ld\n", mlx->calib_data_.Ea_));
  MLX_INFO(("mlx->calib_data_.Eb_ = %ld\n", mlx->calib_data_.Eb_));
  MLX_INFO(("mlx->calib_data_.Fa_ = %ld\n", mlx->calib_data_.Fa_));
  MLX_INFO(("mlx->calib_data_.Fb_ = %ld\n", mlx->calib_data_.Fb_));
  MLX_INFO(("mlx->calib_data_.Ga_ = %ld\n", mlx->calib_data_.Ga_));
  MLX_INFO(("mlx->calib_data_.Gb_ = %d\n", mlx->calib_data_.Gb_));
  MLX_INFO(("mlx->calib_data_.Ka_ = %d\n", mlx->calib_data_.Ka_));
  MLX_INFO(("mlx->calib_data_.Ha_ = %d\n", mlx->calib_data_.Ha_));
  MLX_INFO(("mlx->calib_data_.Hb_ = %d\n", mlx->calib_data_.Hb_));
}


void
_mlx90632_print_adc_data (struct Mlx90632Device *mlx)
{
  MLX_INFO(("mlx->adc_data_.RAM_9_ = %d\n", mlx->adc_data_.RAM_9_));
  MLX_INFO(("mlx->adc_data_.RAM_4_ = %d\n", mlx->adc_data_.RAM_4_));
  MLX_INFO(("mlx->adc_data_.RAM_5_ = %d\n", mlx->adc_data_.RAM_5_));
  MLX_INFO(("mlx->adc_data_.RAM_6_ = %d\n", mlx->adc_data_.RAM_6_));
  MLX_INFO(("mlx->adc_data_.RAM_7_ = %d\n", mlx->adc_data_.RAM_7_));
  MLX_INFO(("mlx->adc_data_.RAM_8_ = %d\n", mlx->adc_data_.RAM_8_));
}


#ifdef MLX_HUB_PRINT_ENABLE
  #ifdef ARDUINO
    #include <stdarg.h>
    #include <stdio.h>
    #include <Arduino.h>
    void serial_printf(const char *args, ...)
    {
      static char print_buffer[256];
      va_list argp;
      va_start(argp, args);
      vsprintf(print_buffer, args, argp);
      Serial.print(print_buffer);
      va_end(argp);
    }
  #endif
#endif

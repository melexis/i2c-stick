#include "mlx90394_api.h"
#include "mlx90394_hal.h"
#include <Arduino.h> // this is here only to debug for "Serial" object... please remove this include!


int
mlx90394_init(uint8_t sa)
{
  return 0;
}


int
mlx90394_write_measurement_mode(uint8_t sa, uint8_t mode)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_CTRL1, &data, 1);
  if (result != 0)
  {
    return -1;
  }

  data &= 0x00F0;
  data |= (mode & 0x000F);
  return mlx90394_i2c_addressed_write(sa, MLX90394_CTRL1, data);
}


int
mlx90394_trigger_measurement(uint8_t sa)
{
  return mlx90394_write_measurement_mode(sa, MLX90394_MODE_SINGLE);
}


int
mlx90394_read_data_ready_bit(uint8_t sa)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_STAT1, &data, 1);
  if (result != 0)
  {
    return -1;
  }
  if (data & MLX90394_STAT1_MASK_DRDY)
  {
    return 1;
  }
  return 0;
}


int
mlx90394_read_xyzt(uint8_t sa, int16_t *x, int16_t *y, int16_t *z, int16_t *t)
{
  uint16_t data[10];
  int result = mlx90394_i2c_addressed_read(sa, 0x00, data, 10);

  if (result != 0)
  {
    return -1;
  }
  *x = data[0x01] | (data[0x02] << 8);
  *y = data[0x03] | (data[0x04] << 8);
  *z = data[0x05] | (data[0x06] << 8);
  *t = data[0x08] | (data[0x09] << 8);
  return 0;
}


int
mlx90394_measure_xyzt(uint8_t sa, int16_t *x, int16_t *y, int16_t *z, int16_t *t, int16_t timeout_us)
{
  return -1;
}

int
mlx90394_write_EN_X(uint8_t sa, uint8_t enable)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_CTRL1, &data, 1);
  if (result != 0)
  {
    return -1;
  }

  data &= ~MLX90394_CTRL1_MASK_EN_X;
  if (enable)
  {
    data |= MLX90394_CTRL1_MASK_EN_X;
  }
  result = mlx90394_i2c_addressed_write(sa, MLX90394_CTRL1, data);
  return result;
}


int
mlx90394_write_EN_Y(uint8_t sa, uint8_t enable)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_CTRL1, &data, 1);
  if (result != 0)
  {
    return -1;
  }

  data &= ~MLX90394_CTRL1_MASK_EN_Y;
  if (enable)
  {
    data |= MLX90394_CTRL1_MASK_EN_Y;
  }
  result = mlx90394_i2c_addressed_write(sa, MLX90394_CTRL1, data);
  return result;
}


int
mlx90394_write_EN_Z(uint8_t sa, uint8_t enable)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_CTRL1, &data, 1);
  if (result != 0)
  {
    return -1;
  }

  data &= ~MLX90394_CTRL1_MASK_EN_Z;
  if (enable)
  {
    data |= MLX90394_CTRL1_MASK_EN_Z;
  }
  result = mlx90394_i2c_addressed_write(sa, MLX90394_CTRL1, data);
  return result;
}


int
mlx90394_write_EN_T(uint8_t sa, uint8_t enable)
{
  uint16_t data = 0; 
  int result = mlx90394_i2c_addressed_read(sa, MLX90394_CTRL4, &data, 1);
  if (result != 0)
  {
    return -1;
  }

  data &= ~MLX90394_CTRL4_MASK_EN_T;
  if (enable)
  {
    data |= MLX90394_CTRL4_MASK_EN_T;
  }
  result = mlx90394_i2c_addressed_write(sa, MLX90394_CTRL4, data);
  return result;
}

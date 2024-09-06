#include "mlx90394_cmd.h"
#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_dispatcher.h"
#include "i2c_stick_hal.h"
#include "mlx90394_hal.h"
#include "mlx90394_api.h"

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MLX90394_SLAVES
#define MAX_MLX90394_SLAVES 8
#endif // MAX_MLX90394_SLAVES

#define MLX90394_ERROR_BUFFER_TOO_SMALL "Buffer too small"
#define MLX90394_ERROR_COMMUNICATION "Communication error"
#define MLX90394_ERROR_NO_FREE_HANDLE "No free handle; pls recompile firmware with higher 'MAX_MLX90394_SLAVES'"
#define MLX90394_ERROR_OUT_OF_RANGE "Out of range"


static MLX90394_t *g_mlx90394_list[MAX_MLX90394_SLAVES];


MLX90394_t *
cmd_90394_get_handle(uint8_t sa)
{
  if (sa >= 128)
  {
    return NULL;
  }

  for (uint8_t i=0; i<MAX_MLX90394_SLAVES; i++)
  {
    if (!g_mlx90394_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90394_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      return g_mlx90394_list[i];
    }
  }

  // not found => try to find a handle with slave address zero (not yet initialized)!
  for (uint8_t i=0; i<MAX_MLX90394_SLAVES; i++)
  {
    if (!g_mlx90394_list[i])
    {
      continue; // allow empty spots!
    }
    if (g_mlx90394_list[i]->slave_address_ == 0)
    { // found!
      return g_mlx90394_list[i];
    }
  }

  // not found => use first free spot!
  uint8_t i=0;
  for (; i<MAX_MLX90394_SLAVES; i++)
  {
    if (g_mlx90394_list[i] == NULL)
    {
      g_mlx90394_list[i] = (MLX90394_t *)malloc(sizeof(MLX90394_t));;
      memset(g_mlx90394_list[i], 0, sizeof(MLX90394_t));
      g_mlx90394_list[i]->slave_address_ = 0x80 | sa;
      return g_mlx90394_list[i];
    }
  }

  return NULL; // no free spot available
}


static void
delete_handle(uint8_t sa)
{
  for (uint8_t i=0; i<MAX_MLX90394_SLAVES; i++)
  {
    if (!g_mlx90394_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90394_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      memset(g_mlx90394_list[i], 0, sizeof(MLX90394_t));
      free(g_mlx90394_list[i]);
      g_mlx90394_list[i] = NULL;
    }
  }
}


int16_t
cmd_90394_register_driver()
{
  int16_t r = 0;

  r = i2c_stick_register_driver(0x60, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x61, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x10, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x11, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x68, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x69, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x6a, DRV_MLX90394_ID);
  if (r < 0) return r;

  r = i2c_stick_register_driver(0x6b, DRV_MLX90394_ID);
  if (r < 0) return r;

  return 1;
}


void
cmd_90394_init(uint8_t sa)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  // init functions goes here
  mlx90394_init(sa);

  mlx90394_write_measurement_mode(sa, MLX90394_MODE_100Hz);
  mlx->mode_ = MLX90394_MODE_100Hz;
  mlx90394_write_EN_X(sa);
  mlx->enable_values_ |= 0x01;
  mlx90394_write_EN_Y(sa);
  mlx->enable_values_ |= 0x02;
  mlx90394_write_EN_Z(sa);
  mlx->enable_values_ |= 0x04;
  mlx90394_write_EN_T(sa);
  mlx->enable_values_ |= 0x08;

  // turn off bit7, to indicate other routines this slave has been init
  mlx->slave_address_ &= 0x7F;
}


void
cmd_90394_tear_down(uint8_t sa)
{ // nothing special to do, just release all associated memory
  delete_handle(sa);
}


void
cmd_90394_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *mv_count = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  if (*mv_count < 4) // check Measurement Value buffer length
  {
    *mv_count = 0;
    *error_message = MLX90394_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *mv_count = 0;

  while (mlx90394_read_data_ready_bit(sa) == 0)
  {
    mlx90394_delay_us(100);
  }

  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  int16_t t = 0;
  int result = mlx90394_read_xyzt(sa, &x, &y, &z, &t);
  if (result != 0)
  {
    *mv_count = 0;
    *error_message = MLX90394_ERROR_COMMUNICATION;
    return;
  }

  uint8_t i = 0;
  if (mlx->enable_values_ & 0x01)
  {
    mv_list[i] = x;
    i++;
  }
  if (mlx->enable_values_ & 0x02)
  {
    mv_list[i] = y;
    i++;
  }
  if (mlx->enable_values_ & 0x04)
  {
    mv_list[i] = z;
    i++;
  }
  if (mlx->enable_values_ & 0x08)
  {
    mv_list[i] = t;
    i++;
  }
  *mv_count = i;
}


void
cmd_90394_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *raw_count = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  if (*raw_count < 4) // check Raw Value buffer length
  {
    *raw_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90394_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *raw_count = 4;

  while (mlx90394_read_data_ready_bit(sa) == 0)
  {
    mlx90394_delay_us(100);
  }

  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  int16_t t = 0;
  int result = mlx90394_read_xyzt(sa, &x, &y, &z, &t);
  if (result != 0)
  {
    *raw_count = 0;
    *error_message = MLX90394_ERROR_COMMUNICATION;
    return;
  }
  raw_list[0] = x;
  raw_list[1] = y;
  raw_list[2] = z;
  raw_list[3] = t;
}


void
cmd_90394_nd(uint8_t sa, uint8_t *nd, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *nd = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  int result = mlx90394_read_data_ready_bit(sa);
  if (result < 0)
  {
    *nd = 0;
    *error_message = MLX90394_ERROR_COMMUNICATION;
    return;
  }

  *nd = 0;
  if (result > 0)
  {
    *nd = 1;
    return;
  }
}


void
cmd_90394_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *sn_count = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  if (*sn_count < 4) // check the Serial Number buffer length
  {
    *sn_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90394_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *sn_count = 4;

  // todo:
  //
  // read the serial number from the sensor.
  //
}


void
cmd_90394_cs(uint8_t sa, uint8_t channel_mask, const char *input)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  // todo:
  //
  // read the CS(Configuration of the Slave) from the sensor.
  //

  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":MODE=", 0);
  itoa(mlx->mode_, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);

  const char *p = "(Unknown)";
  if (mlx->mode_ == MLX90394_MODE_POWER_DOWN ) p = "(POWER_DOWN)";
  if (mlx->mode_ == MLX90394_MODE_SINGLE     ) p = "(SINGLE)";
  if (mlx->mode_ == MLX90394_MODE_5Hz        ) p = "(5Hz)";
  if (mlx->mode_ == MLX90394_MODE_10Hz       ) p = "(10Hz)";
  if (mlx->mode_ == MLX90394_MODE_15Hz       ) p = "(15Hz)";
  if (mlx->mode_ == MLX90394_MODE_50Hz       ) p = "(50Hz)";
  if (mlx->mode_ == MLX90394_MODE_100Hz      ) p = "(100Hz)";
  if (mlx->mode_ == MLX90394_MODE_SELF_TEST  ) p = "(SELF_TEST)";
  if (mlx->mode_ == MLX90394_MODE_POWER_DOWN2) p = "(POWER_DOWN)";
  if (mlx->mode_ == MLX90394_MODE_SINGLE2    ) p = "(SINGLE)";
  if (mlx->mode_ == MLX90394_MODE_200Hz      ) p = "(200Hz)";
  if (mlx->mode_ == MLX90394_MODE_500Hz      ) p = "(500Hz)";
  if (mlx->mode_ == MLX90394_MODE_700Hz      ) p = "(700Hz)";
  if (mlx->mode_ == MLX90394_MODE_1000Hz     ) p = "(1000Hz)";
  if (mlx->mode_ == MLX90394_MODE_1400Hz     ) p = "(1400Hz)";
  if (mlx->mode_ == MLX90394_MODE_POWER_DOWN3) p = "(POWER_DOWN)";
  send_answer_chunk(channel_mask, p, 1);


  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":EN_X=", 0);
  itoa((mlx->enable_values_ & 0x01) ? 1 : 0, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":EN_Y=", 0);
  itoa((mlx->enable_values_ & 0x02) ? 1 : 0, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":EN_Z=", 0);
  itoa((mlx->enable_values_ & 0x04) ? 1 : 0, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":EN_T=", 0);
  itoa((mlx->enable_values_ & 0x08) ? 1 : 0, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  // todo:
  //
  // Send the configuration of the MV header, unit and resolution back to the terminal(not to sensor!)
  //

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_HEADER=", 0);
  uint8_t value_count = 0;
  if (mlx->enable_values_ & 0x01)
  {
    send_answer_chunk(channel_mask, "X", 0);
    value_count++;
  }
  if (mlx->enable_values_ & 0x02)
  {
    if (value_count > 0)
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "Y", 0);
    value_count++;
  }
  if (mlx->enable_values_ & 0x04)
  {
    if (value_count > 0)
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "Z", 0);
    value_count++;
  }
  if (mlx->enable_values_ & 0x08)
  {
    if (value_count > 0)
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "T", 0);
    value_count++;
  }
  send_answer_chunk(channel_mask, "", 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_UNIT=", 0);
  for (uint8_t i=0; i<value_count; i++)
  {
    if (i == 0)
    {
      send_answer_chunk(channel_mask, "LSB", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",LSB", 0);
    }
  }
  send_answer_chunk(channel_mask, "", 1);
}


void
cmd_90394_cs_write(uint8_t sa, uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }


  //
  // todo:
  //
  // write the configuration of the slave to the sensor and report to the channel the status.
  //
  // Please get inspired from other drivers like MLX90614.
  //
  // Also if SA can be re-programmed, please add the correct sequence here, see also MLX90614 or MLX90632 for an extensive example.
  //

  const char *var_name = "MODE=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *v = input+strlen(var_name);
    int16_t mode = -1;
    if (!strcmp(v, "POWER_DOWN")) mode = MLX90394_MODE_POWER_DOWN;
    if (!strcmp(v, "SINGLE"))     mode = MLX90394_MODE_SINGLE;
    if (!strcmp(v, "5Hz"))        mode = MLX90394_MODE_5Hz;
    if (!strcmp(v, "10Hz"))       mode = MLX90394_MODE_10Hz;
    if (!strcmp(v, "15Hz"))       mode = MLX90394_MODE_15Hz;
    if (!strcmp(v, "50Hz"))       mode = MLX90394_MODE_50Hz;
    if (!strcmp(v, "100Hz"))      mode = MLX90394_MODE_100Hz;
    if (!strcmp(v, "SELF_TEST"))  mode = MLX90394_MODE_SELF_TEST;
    if (!strcmp(v, "200Hz"))      mode = MLX90394_MODE_200Hz;
    if (!strcmp(v, "500Hz"))      mode = MLX90394_MODE_500Hz;
    if (!strcmp(v, "700Hz"))      mode = MLX90394_MODE_700Hz;
    if (!strcmp(v, "1000Hz"))     mode = MLX90394_MODE_1000Hz;
    if (!strcmp(v, "1400Hz"))     mode = MLX90394_MODE_1400Hz;
    if (mode < 0)
    {
      mode = atoi(v);
    }
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((mode >= 0) && (mode <= 15))
    {
      mlx->mode_ = mode;
      mlx90394_write_measurement_mode(sa, mode);
      send_answer_chunk(channel_mask, ":MODE=OK [mlx-IO]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":MODE=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "EN_X=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *v = input+strlen(var_name);
    int16_t en = atoi(v);
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((en >= 0) && (en <= 1))
    {
      mlx->enable_values_ &= ~0x01;
      if (en)
      {
        mlx->enable_values_ |= 0x01;
      }
      mlx90394_write_EN_X(sa);
      send_answer_chunk(channel_mask, ":EN_X=OK [mlx-IO]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EN_X=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "EN_Y=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *v = input+strlen(var_name);
    int16_t en = atoi(v);
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((en >= 0) && (en <= 1))
    {
      mlx->enable_values_ &= ~0x02;
      if (en)
      {
        mlx->enable_values_ |= 0x02;
      }
      mlx90394_write_EN_Y(sa);
      send_answer_chunk(channel_mask, ":EN_Y=OK [mlx-IO]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EN_Y=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "EN_Z=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *v = input+strlen(var_name);
    int16_t en = atoi(v);
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((en >= 0) && (en <= 1))
    {
      mlx->enable_values_ &= ~0x04;
      if (en)
      {
        mlx->enable_values_ |= 0x04;
      }
      mlx90394_write_EN_Z(sa);
      send_answer_chunk(channel_mask, ":EN_Z=OK [mlx-IO]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EN_Z=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "EN_T=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *v = input+strlen(var_name);
    int16_t en = atoi(v);
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((en >= 0) && (en <= 1))
    {
      mlx->enable_values_ &= ~0x08;
      if (en)
      {
        mlx->enable_values_ |= 0x08;
      }
      mlx90394_write_EN_T(sa);
      send_answer_chunk(channel_mask, ":EN_T=OK [mlx-IO]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EN_T=FAIL; outbound", 1);
    }
    return;
  }

  // finally we have a catch all to inform the user that they asked something unknown.
  send_answer_chunk(channel_mask, "+cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":FAIL; unknown variable", 1);
}


void
cmd_90394_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  // indicate 8 bit at each single address:
  *bit_per_address = 8;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90394_ERROR_OUT_OF_RANGE;
    return;
  }

  int result = mlx90394_i2c_addressed_read(sa, mem_start_address, mem_data, mem_count);
  if (result != 0)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90394_ERROR_COMMUNICATION;
    return;
  }
}


void
cmd_90394_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90394_t *mlx = cmd_90394_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90394_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90394_init(sa);
  }

  *bit_per_address = 8;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90394_ERROR_OUT_OF_RANGE;
    return;
  }

  for (uint16_t i=0; i<mem_count; i++)
  {
    uint16_t addr = mem_start_address+i;
    // todo:
    //
    // check if write to EEPROM, add write delay...
    // MW(Memory Write) is able to write anywhere, except the (mlx-)locked areas
    //

    int result = mlx90394_i2c_addressed_write(sa, addr, mem_data[i]);

    if (result < 0)
    {
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90394_ERROR_COMMUNICATION;
      return;
    }
  }
}


void
cmd_90394_is(uint8_t sa, uint8_t *is_ok, char const **error_message)
{ // function to call prior any init, only to check is the connected slave IS a MLX90614.
  uint16_t value;
  *is_ok = 1; // be optimistic!

  // todo:
  //
  // find a way to verify the connected slave at <sa> slave address is actually a MLX90394!
  // often times this can be done by reading some specific values from the ROM or EEPROM,
  // and verify the values are as expected.
  //

  // remember there is no communication initiated yet...

  mlx90394_i2c_init();

  uint8_t device_id = 0x00;
  uint8_t company_id = 0x00;

  mlx90394_i2c_addressed_read(sa, MLX90394_COMPANY_ID, &value, 1);
  company_id = value;
  mlx90394_i2c_addressed_read(sa, MLX90394_DEVICE_ID, &value, 1);
  device_id = value;

  if (company_id != 0x94)
  {
    *error_message = MLX90394_ERROR_COMMUNICATION;
    *is_ok = 0;
    return;
  }
  if (device_id != 0xAA)
  {
    *error_message = MLX90394_ERROR_COMMUNICATION;
    *is_ok = 0;
    return;
  }
}


#ifdef __cplusplus
}
#endif

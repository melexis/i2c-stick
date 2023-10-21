#include "mlx90621_cmd.h"
#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_dispatcher.h"
#include "i2c_stick_hal.h"

#include "mlx90621_api.h"
#include "mlx90621_i2c_driver.h"

#include <string.h>
#include <stdlib.h>

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MLX90621_SLAVES
#define MAX_MLX90621_SLAVES 8
#endif // MAX_MLX90621_SLAVES

#define MLX90621_ERROR_BUFFER_TOO_SMALL "Buffer too small"
#define MLX90621_ERROR_COMMUNICATION "Communication error"
#define MLX90621_ERROR_NO_FREE_HANDLE "No free handle; pls recompile firmware with higher 'MAX_MLX90621_SLAVES'"
#define MLX90621_ERROR_OUT_OF_RANGE "Out of range"


static MLX90621_t *g_mlx90621_list[MAX_MLX90621_SLAVES];


MLX90621_t *
cmd_90621_get_handle(uint8_t sa)
{
  if (sa >= 128)
  {
    return NULL;
  }

  for (uint8_t i=0; i<MAX_MLX90621_SLAVES; i++)
  {
    if (!g_mlx90621_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90621_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      return g_mlx90621_list[i];
    }
  }

  // not found => try to find a handle with slave address zero (not yet initialized)!
  for (uint8_t i=0; i<MAX_MLX90621_SLAVES; i++)
  {
    if (!g_mlx90621_list[i])
    {
      continue; // allow empty spots!
    }
    if (g_mlx90621_list[i]->slave_address_ == 0)
    { // found!
      return g_mlx90621_list[i];
    }
  }

  // not found => use first free spot!
  uint8_t i=0;
  for (; i<MAX_MLX90621_SLAVES; i++)
  {
    if (g_mlx90621_list[i] == NULL)
    {
      g_mlx90621_list[i] = (MLX90621_t *)malloc(sizeof(MLX90621_t));;
      memset(g_mlx90621_list[i], 0, sizeof(MLX90621_t));
      g_mlx90621_list[i]->slave_address_ = 0x80 | sa;
      return g_mlx90621_list[i];
    }
  }

  return NULL; // no free spot available
}


static void
delete_handle(uint8_t sa)
{
  for (uint8_t i=0; i<MAX_MLX90621_SLAVES; i++)
  {
    if (!g_mlx90621_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90621_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      memset(g_mlx90621_list[i], 0, sizeof(MLX90621_t));
      free(g_mlx90621_list[i]);
      g_mlx90621_list[i] = NULL;
    }
  }
}


int16_t
cmd_90621_register_driver()
{
  int16_t r = 0;

  r = i2c_stick_register_driver(0x60, DRV_MLX90621_ID);
  if (r < 0) return r;

  return 1;
}


static int8_t 
check_90621_calibration_ranges(paramsMLX90621 *mlx90621)
{  
  // those ranges are experimental; one might require to tweak those...

  // Serial.printf("mlx90621->vTh25: %d\n", mlx90621->vTh25);
  // Serial.printf("mlx90621->kT1: %f\n", mlx90621->kT1);
  // Serial.printf("mlx90621->kT2: %f\n", mlx90621->kT2);
  // Serial.printf("mlx90621->tgc: %f\n", mlx90621->tgc);
  // Serial.printf("mlx90621->KsTa: %f\n", mlx90621->KsTa);
  // Serial.printf("mlx90621->ksTo: %f\n", mlx90621->ksTo);
  // for (int i=0; i<64; i++)
  // {
  //   Serial.printf("mlx90621->alpha[%d]: %fe-6\n", i, (mlx90621->alpha[i] * 1000000.0));
  // }

  if ((mlx90621->vTh25 < 20000) ||
      (mlx90621->vTh25 > 30000))
  {
    return 1;    
  }
  if ((mlx90621->kT1 < 70.0) ||
      (mlx90621->kT1 > 100.0))
  {
    return 1;
  }
  if ((mlx90621->kT2  < -0.1) ||
      (mlx90621->kT2 > 0.1))
  {
    return 1;
  }
  if ((mlx90621->tgc < 0.25) ||
      (mlx90621->tgc > 2.00))
  {
    return 1;
  }
  if ((mlx90621->KsTa < 0.0001) ||
      (mlx90621->KsTa > 0.0100))
  {
    return 1;
  }
  if ((mlx90621->ksTo < -0.0100) ||
      (mlx90621->ksTo > -0.0001))
  {
    return 1;
  }

  for (int i=0; i<64; i++)
  {
    if ((mlx90621->alpha[i] < 0.001E-6) ||
        (mlx90621->alpha[i] > 2.0E-6))
    {
      return 1;
    }
  }  
  return 0;
}


void
cmd_90621_init(uint8_t sa)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  // init functions goes here
  mlx->slave_address_ = sa;

  mlx->emissivity_ = 0.95;
  mlx->tr_ = 25.0;

  uint8_t eeMLX90621[256];

  MLX90621_I2CInit();
  MLX90621_DumpEE (eeMLX90621);
  MLX90621_Configure(eeMLX90621);
  MLX90621_SetRefreshRate (0x09); // 32Hz
  MLX90621_ExtractParameters(eeMLX90621, &mlx->mlx90621_);

  // turn off bit7, to indicate other routines this slave has been init
  mlx->slave_address_ &= 0x7F;
}


void
cmd_90621_tear_down(uint8_t sa)
{ // nothing special to do, just release all associated memory
  delete_handle(sa);
}


void
cmd_90621_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *mv_count = 0;
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  if (*mv_count <= 65) // check Measurement Value buffer length
  {
    *mv_count = 0;
    *error_message = MLX90621_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *mv_count = 65;

  uint16_t mlx90621Frame[66];
  memset(&mlx90621Frame, 0, sizeof(mlx90621Frame));
  MLX90621_GetFrameData (mlx90621Frame);

  mlx->ta_ = MLX90621_GetTa (mlx90621Frame, &mlx->mlx90621_);
  mv_list[0] = mlx->ta_;

  MLX90621_CalculateTo(mlx90621Frame, &mlx->mlx90621_, mlx->emissivity_, mlx->tr_, mlx->to_);
  for (int i=0; i<64; i++)
  {
    mv_list[i+1] = mlx->to_[i];
  }
}


void
cmd_90621_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *raw_count = 0;
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  if (*raw_count < 66) // check Raw Value buffer length
  {
    *raw_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90621_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *raw_count = 66;

  uint16_t mlx90621Frame[66];
  memset(&mlx90621Frame, 0, sizeof(mlx90621Frame));
  MLX90621_GetFrameData (mlx90621Frame);
  for (uint16_t i=0; i<66; i++)
  {
    raw_list[i] = (uint16_t)mlx90621Frame[i];
  }
}


void
cmd_90621_nd(uint8_t sa, uint8_t *nd, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  *nd = 1; // not implemented, but there is 'always' new data...

  // todo:
  //
  // read the status from the sensor and check if new data is available (ND=New Data).
  //
 }


void
cmd_90621_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *sn_count = 0;
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  if (*sn_count < 4) // check the Serial Number buffer length
  {
    *sn_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90621_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *sn_count = 4;

  sn_list[0] = 0;
  sn_list[1] = 0;
  sn_list[2] = 0;
  sn_list[3] = 0;

  // todo:
  //
  // read the serial number from the sensor.
  //
}


void
cmd_90621_cs(uint8_t sa, uint8_t channel_mask, const char *input)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  // todo:
  //
  // read the CS(Configuration of the Slave) from the sensor.
  //

  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":SA=", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 1);


  // todo:
  //
  // Send the answer back in the format
  // "cs:<sa>:<key>=<value>"
  //


  // todo:
  //
  // Send the configuration of the MV header, unit and resolution back to the terminal(not to sensor!)
  //

  // send_answer_chunk(channel_mask, "cs:", 0);
  // uint8_to_hex(buf, sa);
  // send_answer_chunk(channel_mask, buf, 0);
  // send_answer_chunk(channel_mask, ":RO:MV_HEADER=TA,TO", 1);

  // send_answer_chunk(channel_mask, "cs:", 0);
  // uint8_to_hex(buf, sa);
  // send_answer_chunk(channel_mask, buf, 0);
  // send_answer_chunk(channel_mask, ":RO:MV_UNIT=DegC,DegC", 1);

  // send_answer_chunk(channel_mask, "cs:", 0);
  // uint8_to_hex(buf, sa);
  // send_answer_chunk(channel_mask, buf, 0);
  // send_answer_chunk(channel_mask, ":RO:MV_RES=" xstr(MLX90614_LSB_C) "," xstr(MLX90614_LSB_C), 1);
}


void
cmd_90621_cs_write(uint8_t sa, uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
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

  // finally we have a catch all to inform the user that they asked something unknown.
  send_answer_chunk(channel_mask, "+cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":FAIL; unknown variable", 1);
}


void
cmd_90621_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  // indicate 16 bit at each single address:
  *bit_per_address = 16;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90621_ERROR_OUT_OF_RANGE;
    return;
  }

  for (uint16_t i=0; i<mem_count; i++)
  {
  	// todo: read memory from the sensor!

    //int32_t result = MLX90614_SMBusRead(sa, mem_start_address + i, &mem_data[i]);

    //if (result != 0)
    {
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90621_ERROR_COMMUNICATION;
      return;
    }
  }
}


void
cmd_90621_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90621_t *mlx = cmd_90621_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90621_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90621_init(sa);
  }

  *bit_per_address = 16;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90621_ERROR_OUT_OF_RANGE;
    return;
  }

  for (uint16_t i=0; i<mem_count; i++)
  {
    uint16_t addr = mem_start_address+i;
    // todo:
    //
    // check if write to EEPROM, the procedure is correct here.
    // MW(Memory Write) is able to write anywhere, except the (mlx-)locked areas
    //

    // int result = MLX90614_SMBusWrite(sa, addr, mem_data[i]);

    // if (result < 0)
    {
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90621_ERROR_COMMUNICATION;
      return;
    }
  }
}


void
cmd_90621_is(uint8_t sa, uint8_t *is_ok, char const **error_message)
{ // function to call prior any init, only to check is the connected slave IS a MLX90614.
  uint16_t value;
  *is_ok = 1; // be optimistic!

  uint8_t eeMLX90621[256];
  paramsMLX90621 mlx90621;
  int status;

  MLX90621_I2CInit();
  status = MLX90621_DumpEE (eeMLX90621);
  if (status != 0)
  {
    *is_ok = 0;
    return;
  }

  status = MLX90621_ExtractParameters(eeMLX90621, &mlx90621);
  if (status != 0)
  {
    *is_ok = 0;
    return;
  }

  status = check_90621_calibration_ranges(&mlx90621);
  if (status != 0)
  {
    *is_ok = 0;
    return;
  }
}


#ifdef __cplusplus
}
#endif

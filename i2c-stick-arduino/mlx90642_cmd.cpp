#include <Arduino.h>
#include "mlx90642_cmd.h"
#include "mlx90642.h"
#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_dispatcher.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MLX90642_SLAVES
#define MAX_MLX90642_SLAVES 2
#endif // MAX_MLX90642_SLAVES

#define MLX90642_ERROR_BUFFER_TOO_SMALL "Buffer too small"
#define MLX90642_ERROR_COMMUNICATION "Communication error"
#define MLX90642_ERROR_NO_FREE_HANDLE "No free handle; pls recompile firmware with higher 'MAX_MLX90642_SLAVES'"
#define MLX90642_ERROR_OUT_OF_RANGE "Out of range"

#define MLX90642_LSB_SENSOR_C 100
#define MLX90642_LSB_OBJECT_C 50


static MLX90642_t *g_mlx90642_list[MAX_MLX90642_SLAVES];


float MLX90642_EM_to_float(int16_t emissivity)
{
	return float(emissivity) / 0x4000;
}

int16_t MLX90642_EM_to_int(float emissivity)
{
	return int16_t(emissivity * 0x4000);
}


MLX90642_t *
cmd_90642_get_handle(uint8_t sa)
{
  if (sa >= 128)
  {
    return NULL;
  }

  for (uint8_t i=0; i<MAX_MLX90642_SLAVES; i++)
  {
    if (!g_mlx90642_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90642_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      return g_mlx90642_list[i];
    }
  }

  // not found => try to find a handle with slave address zero (not yet initialized)!
  for (uint8_t i=0; i<MAX_MLX90642_SLAVES; i++)
  {
    if (!g_mlx90642_list[i])
    {
      continue; // allow empty spots!
    }
    if (g_mlx90642_list[i]->slave_address_ == 0)
    { // found!
      return g_mlx90642_list[i];
    }
  }

  // not found => use first free spot!
  uint8_t i=0;
  for (; i<MAX_MLX90642_SLAVES; i++)
  {
    if (g_mlx90642_list[i] == NULL)
    {
      g_mlx90642_list[i] = (MLX90642_t *)malloc(sizeof(MLX90642_t));;
      memset(g_mlx90642_list[i], 0, sizeof(MLX90642_t));
      g_mlx90642_list[i]->slave_address_ = 0x80 | sa;
      return g_mlx90642_list[i];
    }
  }

  return NULL; // no free spot available
}


static void
delete_handle(uint8_t sa)
{
  for (uint8_t i=0; i<MAX_MLX90642_SLAVES; i++)
  {
    if (!g_mlx90642_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90642_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      memset(g_mlx90642_list[i], 0, sizeof(MLX90642_t));
      free(g_mlx90642_list[i]);
      g_mlx90642_list[i] = NULL;
    }
  }
}


int16_t
cmd_90642_register_driver()
{
  int16_t r = 0;

  r = i2c_stick_register_driver(0x66, DRV_MLX90642_ID);
  if (r < 0) return r;

  return 1;
}


void
cmd_90642_init(uint8_t sa)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  // init functions goes here

  // turn off bit7, to indicate other routines this slave has been init
  mlx->slave_address_ &= 0x7F;
}


void
cmd_90642_tear_down(uint8_t sa)
{ // nothing special to do, just release all associated memory
  delete_handle(sa);
}



void
cmd_90642_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *mv_count = 0;
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }


  if (*mv_count < (768+1)) // check Measurement Value buffer length
  {
    *mv_count = 0;
    *error_message = MLX90642_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *mv_count = (768+1);

  //
  // get the measurement values from the sensor
  //
  int16_t buffer[768];
  uint16_t ta_read;
  int r = MLX90642_GetImage(sa, buffer);
  r = MLX90642_I2CRead(sa, MLX90642_TA_DATA_ADDRESS, 1, &ta_read);

  mv_list[0] = float(int16_t(ta_read)) / MLX90642_LSB_SENSOR_C;
  for (uint16_t pix=0; pix<768; pix++)
  {
    mv_list[1+pix] = float(buffer[pix]) / MLX90642_LSB_OBJECT_C;
  }
}


void
cmd_90642_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *raw_count = 0;
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }

  if (*raw_count < (768+1)) // check Raw Value buffer length
  {
    *raw_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90642_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *raw_count = (768+1);

  //
  // get the raw values from the sensor
  //


  int r = MLX90642_I2CRead(sa, MLX90642_IR_DATA_ADDRESS, 768, &raw_list[1]);
  r = MLX90642_I2CRead(sa, MLX90642_TA_DATA_ADDRESS, 1, &raw_list[0]);
  mlx->progress_bar_ = MLX90642_GetProgress(sa);
}


void
cmd_90642_nd(uint8_t sa, uint8_t *nd, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }

  //
  // read the status from the sensor and check if new data is available (ND=New Data).
  //
  uint16_t value = 0x0000;

  MLX90642_I2CRead(sa, MLX90642_FLAGS_ADDRESS, 1, &value);

  uint8_t busy = (value & 0x0001) ? 1 : 0;
  uint8_t ready = (value & 0x0100) ? 1 : 0;

	*nd = 0; // Be pessimistic, assume there is no new data.
  if (ready)
  {
  	if (busy)
  	{ // this should not be the case, one should not read when DSP is busy, especially during the last 20ms.
  		// let's reset the ready flag, and return that we do not have new data.
  		MLX90642_I2CRead(sa, MLX90642_TO_DATA_ADDRESS, 1, &value);
  		return;
  	}
  	*nd = 1;
  }
}


void
cmd_90642_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *sn_count = 0;
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }

  if (*sn_count < 4) // check the Serial Number buffer length
  {
    *sn_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90642_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *sn_count = 4;

  MLX90642_GetID(sa, sn_list);
}


void
cmd_90642_cs(uint8_t sa, uint8_t channel_mask, const char *input)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
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

	// RR
	int16_t rr = MLX90642_GetRefreshRate(sa);
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RR=", 0);
  itoa(rr, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

	// EM

  int16_t em = 0;
  MLX90642_GetEmissivity(sa, &em);
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":EM=", 0);
  float em_float = MLX90642_EM_to_float(em);
  const char *p = my_dtostrf(em_float, 10, 3, buf);
  while (*p == ' ') p++; // remove leading space
  send_answer_chunk(channel_mask, p, 1);

	// // BGT (BackGround Temperature)
	// int32_t bgt = MLX90642_read_background_temperature(sa);
  // send_answer_chunk(channel_mask, "cs:", 0);
  // uint8_to_hex(buf, sa);
  // send_answer_chunk(channel_mask, buf, 0);
  // send_answer_chunk(channel_mask, ":BGT=", 0);
  // float bgt_float = MLX90642_BGT_to_float(bgt);
  // p = my_dtostrf(bgt_float, 10, 2, buf);
  // while (*p == ' ') p++; // remove leading space
  // send_answer_chunk(channel_mask, p, 1);

	uint8_t major = 0;
	uint8_t minor = 0;
	uint8_t patch = 0;
	MLX90642_GetFWver(sa, &major, &minor, &patch);
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:FW=", 0);
  itoa(major, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ".", 0);
  itoa(minor, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ".", 0);
  itoa(patch, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);


  //
  // Send the configuration of the MV header, unit and resolution back to the terminal
  //
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_HEADER=TA,TO_[768]", 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_UNIT=DegC,DegC[768]", 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_RES=" xstr(MLX90642_LSB_SENSOR_C) "," xstr(MLX90642_LSB_OBJECT_C) "[768]", 1);
}


void
cmd_90642_cs_write(uint8_t sa, uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
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

  const char *var_name = "RR=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t rr = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((rr >= 0) && (rr <= 7))
    {
			MLX90642_SetRefreshRate(sa, rr);
      send_answer_chunk(channel_mask, ":RR=OK [mlx-EE]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":RR=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "EM=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t em = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((-32768 <= em) & (em < 32768)) // check range
    {
			MLX90642_SetEmissivity(sa, em);
      send_answer_chunk(channel_mask, ":EM=OK [mlx-EE]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EM=FAIL; outbound", 1);
    }
    return;
  }

  // var_name = "BGT=";
  // if (!strncmp(var_name, input, strlen(var_name)))
  // {
  //   int32_t bgt = atoi(input+strlen(var_name));
  //   send_answer_chunk(channel_mask, "+cs:", 0);
  //   uint8_to_hex(buf, sa);
  //   send_answer_chunk(channel_mask, buf, 0);
  //   if ((-32768 <= bgt) & (bgt < 32768)) // check range
  //   {
	// 		MLX90642_write_background_temperature(sa, bgt);
  //     send_answer_chunk(channel_mask, ":BGT=OK [mlx-IO]", 1);
  //   } else
  //   {
  //     send_answer_chunk(channel_mask, ":BGT=FAIL; outbound", 1);
  //   }
  //   return;
  // }



  // finally we have a catch all to inform the user that they asked something unknown.
  send_answer_chunk(channel_mask, "+cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":FAIL; unknown variable", 1);
}


void
cmd_90642_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }

  // indicate 16 bit at each single address:
  *bit_per_address = 16;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90642_ERROR_OUT_OF_RANGE;
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
      *error_message = MLX90642_ERROR_COMMUNICATION;
      return;
    }
  }
}


void
cmd_90642_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90642_t *mlx = cmd_90642_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90642_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90642_init(sa);
  }

  *bit_per_address = 16;
  *address_increments = 1;

  if ((mem_start_address + mem_count) > 0x00FF)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90642_ERROR_OUT_OF_RANGE;
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
      *error_message = MLX90642_ERROR_COMMUNICATION;
      return;
    }
  }
}


void
cmd_90642_is(uint8_t sa, uint8_t *is_ok, char const **error_message)
{ // function to call prior any init, only to check is the connected slave IS a MLX90614.
  uint16_t value;
  *is_ok = 1; // be optimistic!

  // todo:
  //
  // find a way to verify the connected slave at <sa> slave address is actually a MLX90642!
  // often times this can be done by reading some specific values from the ROM or EEPROM,
  // and verify the values are as expected.
  //

  // remember there is no communication initiated yet...

  // in this ecample below for MLX90614 we check if the EEPROM reads the slave address at address 0x2E...

//  MLX90614_SMBusInit();
//  if (MLX90614_SMBusRead(sa, 0x2E, &value) < 0)
//  {
//    *error_message = MLX90642_ERROR_COMMUNICATION;
//    *is_ok = 0;
//    return;
//  }
//  if (value & 0x007F != sa)
//  {
//    *is_ok = 0;
//  }
}


#ifdef __cplusplus
}
#endif

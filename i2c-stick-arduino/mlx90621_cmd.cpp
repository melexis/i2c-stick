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
#define MAX_MLX90621_SLAVES 1
#endif // MAX_MLX90621_SLAVES

#define MLX90621_ERROR_BUFFER_TOO_SMALL "Buffer too small"
#define MLX90621_ERROR_COMMUNICATION "Communication error"
#define MLX90621_ERROR_NO_FREE_HANDLE "No free handle; pls recompile firmware with higher 'MAX_MLX90621_SLAVES'"
#define MLX90621_ERROR_OUT_OF_RANGE "Out of range"
#define MLX90621_ERROR_NOT_IMPLEMENTED "Function not implemented"


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

//   Serial.printf("mlx90621->vTh25: %d\n", mlx90621->vTh25);
//   Serial.printf("mlx90621->kT1: %f\n", mlx90621->kT1);
//   Serial.printf("mlx90621->kT2: %f\n", mlx90621->kT2);
//   Serial.printf("mlx90621->tgc: %f\n", mlx90621->tgc);
//   Serial.printf("mlx90621->KsTa: %f\n", mlx90621->KsTa);
//   Serial.printf("mlx90621->ksTo: %f\n", mlx90621->ksTo);
//   for (int i=0; i<64; i++)
//   {
//     Serial.printf("mlx90621->alpha[%d]: %fe-6\n", i, (mlx90621->alpha[i] * 1000000.0));
//   }

  if ((mlx90621->vTh25 < 20000) ||
      (mlx90621->vTh25 > 30000))
  {
    return 1;    
  }
  if ((mlx90621->kT1 < 7.0) ||
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


// function to get the raw data, the frame_data is kept in the full pixel size with original pixel positions.
// the frame_data buffer is initialized with 0x7FFF to indicate the pixel is 'disabled'
static int 
mlx90621_get_frame_data_special(uint16_t *frame_data, uint8_t start_row, uint8_t start_column, uint8_t rows, uint8_t columns)
{
  int error = 1;

  // check input parameters.
  if (rows == 0) return 1;
  if (columns == 0) return 1;
  if (rows > 4) return 1;
  if (columns > 16) return 1;
  if (start_row > 4) start_row = 4;
  if (start_column > 16) start_column = 16;
  if (start_row == 0) start_row = 1;
  if (start_column == 0) start_column = 1;

  if (rows >= 3)
  { // read in column mode, we will likely read too much data, but we save the overhead communication.
    uint8_t start_address = ((start_column-1) * 4);
    uint8_t count = 4 * columns;
    error = MLX90621_I2CRead(0x60, 0x02, start_address, 1, count, frame_data+start_address);
  } else
  { // read in row mode
    for (uint8_t row = start_row-1; row<rows; row++)
    {
      uint8_t start_address = (row * 4);
      uint8_t count = columns;
      error = MLX90621_I2CRead(0x60, 0x02, start_address, 4, count, frame_data+start_address);
    }
  }


  for (uint8_t i=0; i<64; i++)
  {
    uint8_t row = (i % 4) + 1;
    uint8_t col = (i / 4) + 1;
    if ((start_row <= row) && (row < (start_row + rows)))
    {
      if ((start_column <= col) && (col < (start_column + columns)))
      { // this pixel is enabled => skip to set the disable value!
        continue;
      }
    }
    frame_data[i] = 0x7FFF;
  }


  // Serial.printf("\nraw data:\n\n");
  // for (uint8_t row=0; row<4; row++)
  // {
  //   for (uint8_t col=0; col<16; col++)
  //   {
  //     uint8_t i = col * 4 + row;
  //     Serial.printf("%04X  ", frame_data[i]);
  //   }
  //   Serial.printf("\n");  
  // }


  // finally read PTAT & compensation pixel
  error = MLX90621_I2CRead(0x60, 0x02, 0x40, 1, 2, frame_data+0x40);
  return error;
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
  mlx->rows_ = 4;
  mlx->columns_ = 16;
  mlx->start_row_ = 1;
  mlx->start_column_ = 1;

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
  // MLX90621_GetFrameData (mlx90621Frame);
  mlx90621_get_frame_data_special(mlx90621Frame, mlx->start_row_, mlx->start_column_, mlx->rows_, mlx->columns_);

  mlx->ta_ = MLX90621_GetTa (mlx90621Frame, &mlx->mlx90621_);
  mv_list[0] = mlx->ta_;

  MLX90621_CalculateTo(mlx90621Frame, &mlx->mlx90621_, mlx->emissivity_, mlx->tr_, mlx->to_);
  uint16_t result_counter = 0;
  float *p = &mv_list[1];
  for (uint16_t i=0; i<64; i++)
  {
    uint8_t row = (i % 4) + 1;
    uint8_t col = (i / 4) + 1;
    if ((mlx->start_row_ <= row) && (row < (mlx->start_row_ + mlx->rows_)))
    {
      if ((mlx->start_column_ <= col) && (col < (mlx->start_column_ + mlx->columns_)))
      { // this pixel is enabled => add to result list!
        *p++ = mlx->to_[i];
        result_counter++;
        continue;
      }
    }
  }
  *mv_count = (result_counter + 1);
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

  mlx90621_get_frame_data_special(mlx90621Frame, mlx->start_row_, mlx->start_column_, mlx->rows_, mlx->columns_);
  // MLX90621_GetFrameData (mlx90621Frame);

  uint16_t result_counter = 0; 
  uint16_t *p = raw_list;
  for (uint16_t i=0; i<66; i++)
  {
    if (i>=64) // ptat & compensation pix
    {
      *p++ = (uint16_t)mlx90621Frame[i];
      result_counter++;
      continue;
    }
    uint8_t row = (i % 4) + 1;
    uint8_t col = (i / 4) + 1;
    if ((mlx->start_row_ <= row) && (row < (mlx->start_row_ + mlx->rows_)))
    {
      if ((mlx->start_column_ <= col) && (col < (mlx->start_column_ + mlx->columns_)))
      { // this pixel is enabled => add to result list!
        *p++ = (uint16_t)mlx90621Frame[i];
        result_counter++;
        continue;
      }
    }
  }
  *raw_count = result_counter;
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

  //
  // read the serial number from the sensor.
  //
  uint8_t data[8];
  int result = MLX90621_I2CReadEEPROM(0x50, 0xF8, 8, data);
  if (result != 0)
  {
    *error_message = MLX90621_ERROR_COMMUNICATION;
    return;
  }

  for (uint8_t i=0; i<4; i++)
  {
      sn_list[i] = (uint16_t)(data[i*2]) + ((uint16_t)(data[i*2+1]) << 8);
  }
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

  //
  // read the CS(Configuration of the Slave) from the sensor.
  //
  uint8_t rr = MLX90621_GetRefreshRate();
  uint8_t res = MLX90621_GetCurResolution();

  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":SA=", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 1);


  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RR=", 0);
  itoa(rr, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RES=", 0);
  itoa(res, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":ROWS=", 0);
  itoa(mlx->rows_, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":COLUMNS=", 0);
  itoa(mlx->columns_, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":START_ROW=", 0);
  itoa(mlx->start_row_, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":START_COLUMN=", 0);
  itoa(mlx->start_column_, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  //
  // Send the configuration of the MV header, unit and resolution back to the terminal(not to sensor!)
  //

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_HEADER=TA,TO_[", 0);
  itoa(mlx->rows_ * mlx->columns_, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, "]", 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_UNIT=DegC,DegC[", 0);
  itoa(mlx->rows_ * mlx->columns_, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, "]", 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RO:MV_RES=" xstr(MLX90621_LSB_C) "," xstr(MLX90621_LSB_C) "[", 0);
  itoa(mlx->rows_ * mlx->columns_, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, "]", 1);
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
  // write the configuration of the slave to the sensor and report to the channel the status.
  //

  const char *var_name = "EM=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    float em = atof(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((em > 0.1) && (em <= 1.0))
    {
      mlx->emissivity_ = em;
      send_answer_chunk(channel_mask, ":EM=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":EM=FAIL; outbound", 1);
    }
    return;
  }
  var_name = "TR=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    float tr = atof(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((tr >= -60) && (tr <= 100))
    {
      mlx->tr_ = tr;
      send_answer_chunk(channel_mask, ":TR=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":TR=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "RR=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t rr = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((rr >= 0) && (rr <= 15))
    {
      int ret = MLX90621_SetRefreshRate(rr);
      if (ret == 0)
      {
        send_answer_chunk(channel_mask, ":RR=OK [mlx-register]", 1);
      } else
      {
        send_answer_chunk(channel_mask, ":RR=FAIL; communication fail", 1);
      }
    } else
    {
      send_answer_chunk(channel_mask, ":RR=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "RES=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t res = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((res >= 0) && (res <= 3))
    {
      int ret = MLX90621_SetResolution(res);
      if (ret == 0)
      {
        send_answer_chunk(channel_mask, ":RES=OK [mlx-register]", 1);
      } else
      {
        send_answer_chunk(channel_mask, ":RES=FAIL; communication fail", 1);
      }
    } else
    {
      send_answer_chunk(channel_mask, ":RES=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "ROWS=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t rows = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((rows >= 1) && (rows <= 4))
    {
      mlx->rows_ = rows;
      send_answer_chunk(channel_mask, ":ROW=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":ROW=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "COLUMNS=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t columns = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((columns >= 1) && (columns <= 16))
    {
      mlx->columns_ = columns;
      send_answer_chunk(channel_mask, ":COLUMNS=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":COLUMNS=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "START_ROW=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t start_row = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((start_row >= 1) && (start_row <= 4))
    {
      mlx->start_row_ = start_row;
      uint8_t max_rows = 4 - start_row + 1;
      if (mlx->rows_ > max_rows) mlx->rows_ = max_rows;
      send_answer_chunk(channel_mask, ":START_ROW=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":START_ROW=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "START_COLUMN=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t start_column = atoi(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((start_column >= 1) && (start_column <= 16))
    {
      mlx->start_column_ = start_column;
      uint8_t max_columns = 16 - start_column + 1;
      if (mlx->columns_ > max_columns) mlx->columns_ = max_columns;
      send_answer_chunk(channel_mask, ":START_COLUMN=OK [hub-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":START_COLUMN=FAIL; outbound", 1);
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

  bool is_out_of_range = true;

  if ((0x2400 <= mem_start_address) && (mem_start_address < 0x2400+128)) // EEPROM!
  {
    is_out_of_range = false;
    mem_start_address -= 0x2400;
    if (mem_start_address + mem_count > 128) mem_count = 128 - mem_start_address;
    int32_t result = MLX90621_I2CReadEEPROM(0x50, mem_start_address*2, mem_count*2, (uint8_t *)mem_data);
    if (result != 0)
    {
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90621_ERROR_COMMUNICATION;
      return;
    }
  }

  if ((0x4000 <= mem_start_address) && (mem_start_address < (0x4000+256))) // RAM - raw values!
  {
    is_out_of_range = false;
    mem_start_address -= 0x4000;
    if (mem_start_address + mem_count > 256) mem_count = 256 - mem_start_address;
    int result = MLX90621_I2CRead(mlx->slave_address_, 0x02, mem_start_address, 1, mem_count, mem_data);
    if (result != 0)
    {
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90621_ERROR_COMMUNICATION;
      return;
    }
  }

  if (is_out_of_range)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90621_ERROR_OUT_OF_RANGE;
    return;
  }

}


void
cmd_90621_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  *bit_per_address = 0;
  *address_increments = 0;
  *error_message = MLX90621_ERROR_NOT_IMPLEMENTED;
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

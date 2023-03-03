#include "mlx90640_api.h"
#include "mlx90640_i2c_driver.h"
#include "mlx90640_cmd.h"
#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_dispatcher.h"
#include "i2c_stick_hal.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>



#ifdef __cplusplus
extern "C" {
#endif

void cmd_90640_iir_filter(float *to_list, float *iir, uint8_t depth, float threshold);
void cmd_90640_deinterlace_filter(float *to_list, uint8_t subpage);

#ifndef MAX_MLX90640_SLAVES
#define MAX_MLX90640_SLAVES 8
#endif // MAX_MLX90640_SLAVES

#define MLX90640_ERROR_BUFFER_TOO_SMALL "Buffer too small"
#define MLX90640_ERROR_COMMUNICATION "Communication error"
#define MLX90640_ERROR_NEW_DATA_SET "new_data bit set during read"
#define MLX90640_ERROR_NO_FREE_HANDLE "No free handle; pls recompile firmware with higher 'MAX_MLX90640_SLAVES'"

static MLX90640_t *g_mlx90640_list[MAX_MLX90640_SLAVES];


MLX90640_t *
cmd_90640_get_handle(uint8_t sa)
{
  if (sa >= 128)
  {
    return NULL;
  }

  for (uint8_t i=0; i<MAX_MLX90640_SLAVES; i++)
  {
    if (!g_mlx90640_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90640_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      return g_mlx90640_list[i];
    }
  }

  // not found => try to find a handle with slave address zero (not yet initialized)!
  for (uint8_t i=0; i<MAX_MLX90640_SLAVES; i++)
  {
    if (!g_mlx90640_list[i])
    {
      continue; // allow empty spots!
    }
    if (g_mlx90640_list[i]->slave_address_ == 0)
    { // found!
      return g_mlx90640_list[i];
    }
  }
  // not found => use first free spot!
  uint8_t i=0;
  for (; i<MAX_MLX90640_SLAVES; i++)
  {
    if (g_mlx90640_list[i] == NULL)
    {
      g_mlx90640_list[i] = (MLX90640_t *)malloc(sizeof(MLX90640_t));;
      memset(g_mlx90640_list[i], 0, sizeof(MLX90640_t));
      g_mlx90640_list[i]->slave_address_ = 0x80 | sa;
      return g_mlx90640_list[i];
    }
  }

  return NULL; // no free spot available
}


static void
delete_handle(uint8_t sa)
{
  for (uint8_t i=0; i<MAX_MLX90640_SLAVES; i++)
  {
    if (!g_mlx90640_list[i])
    {
      continue; // allow empty spots!
    }
    if ((g_mlx90640_list[i]->slave_address_ & 0x7F) == sa)
    { // found!
      if (g_mlx90640_list[i]->iir_ != NULL)
      {
        free(g_mlx90640_list[i]->iir_);
      }
      memset(g_mlx90640_list[i], 0, sizeof(MLX90640_t));
      free(g_mlx90640_list[i]);
      g_mlx90640_list[i] = NULL;
    }
  }
}


int16_t
cmd_90640_register_driver()
{
  int16_t r = 0;
  r = i2c_stick_register_driver(0x33, DRV_MLX90640_ID);
  if (r < 0) return r;
  return 1;
}


void
cmd_90640_init(uint8_t sa)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  mlx->emissivity_ = 0.95f;
  mlx->t_room_ = 25.0f;
  mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_BROKEN_PIXELS);
  mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_OUTLIER_PIXELS);
  mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_DEINTERLACE_FILTER);
  mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_IIR_FILTER);

  uint16_t ee_data[832];

  MLX90640_I2CInit();
  MLX90640_DumpEE(sa, ee_data);
  MLX90640_ExtractParameters(ee_data, &mlx->mlx90640_);
  MLX90640_SetRefreshRate(sa, 3);
  mlx->slave_address_ &= 0x7F;
}


void
cmd_90640_tear_down(uint8_t sa)
{ // nothing special to do, just release all associated memory
  delete_handle(sa);
}


void
cmd_90640_mv(uint8_t sa, float *mv_list, uint16_t *mv_count, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  uint16_t frame_data[834];
  if (mlx == NULL)
  {
    *mv_count = 0;
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }

  if (*mv_count < 768+1)
  {
    *error_message = MLX90640_ERROR_BUFFER_TOO_SMALL;
    *mv_count = 0;
    return;
  }
  *mv_count = 768+1;

  int e = MLX90640_GetFrameData(sa, frame_data);

  if (e < 0)
  {
    // allow one retry!
    MLX90640_I2CWrite(sa, 0x8000, 0x0030); // clear new data flag...
    e = MLX90640_GetFrameData(sa, frame_data);
    if (e < 0)
    {
      *mv_count = 0;
      *error_message = MLX90640_ERROR_COMMUNICATION;
      if (e == -16)
      {
        *error_message = MLX90640_ERROR_NEW_DATA_SET;
      }
      return;
    }
  }

  if (!(mlx->flags_ & (1U<<MLX90640_CMD_FLAG_IS_INIT)))
  {// make sure 2 subpages are converted at initial call.
    e = MLX90640_GetFrameData(sa, frame_data);

    if (e < 0)
    {
      // allow one retry!
      MLX90640_I2CWrite(sa, 0x8000, 0x0030); // clear new data flag...
      e = MLX90640_GetFrameData(sa, frame_data);
      if (e < 0)
      {
        *mv_count = 0;
        *error_message = MLX90640_ERROR_COMMUNICATION;
        if (e == -16)
        {
          *error_message = MLX90640_ERROR_NEW_DATA_SET;
        }
        return;
      }
    }
  }
  float ta = MLX90640_GetTa(frame_data, &mlx->mlx90640_);
  MLX90640_CalculateTo(frame_data, &mlx->mlx90640_, mlx->emissivity_, mlx->t_room_, mlx->to_list_);


  if ((mlx->flags_ & (1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME)) ||
     !(mlx->flags_ & (1U<<MLX90640_CMD_FLAG_IS_INIT)))
  {
    // trick to compute both sub-pages.
    frame_data[833] ^= 0x0001;
    MLX90640_CalculateTo(frame_data, &mlx->mlx90640_, mlx->emissivity_, mlx->t_room_, mlx->to_list_);
    frame_data[833] ^= 0x0001;
  }

  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_BROKEN_PIXELS))
  {
    int mode = MLX90640_GetCurMode(sa);
    MLX90640_BadPixelsCorrection(mlx->mlx90640_.brokenPixels, mlx->to_list_, mode, &mlx->mlx90640_);
  }

  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_OUTLIER_PIXELS))
  {
    int mode = MLX90640_GetCurMode(sa);
    MLX90640_BadPixelsCorrection(mlx->mlx90640_.outlierPixels, mlx->to_list_, mode, &mlx->mlx90640_);
  }

  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_DEINTERLACE_FILTER))
  {
    if ((mlx->flags_ & (1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME)) ||
       !(mlx->flags_ & (1U<<MLX90640_CMD_FLAG_IS_INIT)))
    { // trick to deinterlace previous sub-pages.
      frame_data[833] ^= 0x0001;
      cmd_90640_deinterlace_filter(mlx->to_list_, frame_data[833]);
      frame_data[833] ^= 0x0001;
    }
    cmd_90640_deinterlace_filter(mlx->to_list_, frame_data[833]);
  }

  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_IIR_FILTER))
  {
    if (mlx->iir_ == NULL)
    {
      mlx->iir_ = (float *)malloc(sizeof(float) * 768);
      for (uint16_t pix=0; pix<768; pix++)
      {
        if ((-100 < mlx->to_list_[pix]) && (mlx->to_list_[pix] < 1000))
        {
          mlx->iir_[pix] = mlx->to_list_[pix];
        } else
        {
          mlx->iir_[pix] = ta;
        }
      }
    }
    cmd_90640_iir_filter(mlx->to_list_, mlx->iir_, 8, 2.5);
    for (uint16_t pix=0; pix<768; pix++)
    {
      mlx->to_list_[pix] = mlx->iir_[pix];
    }
  }

  mv_list[0] = ta;
  for (uint16_t pix=0; pix<768; pix++)
  {
    mv_list[pix+1] = mlx->to_list_[pix];
  }
  mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_IS_INIT);
}


void
cmd_90640_raw(uint8_t sa, uint16_t *raw_list, uint16_t *raw_count, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    *raw_count = 0;
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }
  if (*raw_count < 834)
  {
    *raw_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90640_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *raw_count = 834;
  int e = MLX90640_GetFrameData(sa, raw_list);
  if (e < 0)
  {
    *error_message = MLX90640_ERROR_COMMUNICATION;
    if (e == -16)
    {
      *error_message = MLX90640_ERROR_NEW_DATA_SET;
    }
    return;
  }
}


void
cmd_90640_nd(uint8_t sa, uint8_t *nd, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }
  *nd = 0;

  uint16_t statusRegister;
  int error = MLX90640_I2CRead(sa, 0x8000, 1, &statusRegister);
  if(error != 0)
  {
   *error_message = MLX90640_ERROR_COMMUNICATION;
   return;
  }
  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME))
  {
    *nd = ((statusRegister & 0x0009) == 0x0009) ? 1 : 0;
  } else
  {
    *nd = ((statusRegister & 0x0008) == 0x0008) ? 1 : 0;
  }
}


void
cmd_90640_sn(uint8_t sa, uint16_t *sn_list, uint16_t *sn_count, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    *sn_count = 0;
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }
  if (*sn_count < 3)
  {
    *sn_count = 0; // input buffer not long enough, report nothing.
    *error_message = MLX90640_ERROR_BUFFER_TOO_SMALL;
    return;
  }
  *sn_count = 3;
  if (MLX90640_I2CRead(sa, 0x2407, 3, sn_list))
  {
    *error_message = MLX90640_ERROR_COMMUNICATION;
    return;
  }
}


void
cmd_90640_cs(uint8_t sa, uint8_t channel_mask, const char *input)
{//emissivity - TR - RR - RES - MODE - FLAGS
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }
  uint8_t rr = MLX90640_GetRefreshRate(sa);
  uint8_t res = MLX90640_GetCurResolution(sa);
  uint8_t mode = MLX90640_GetCurMode(sa);


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
  send_answer_chunk(channel_mask, ":EM=", 0);
  const char *p = my_dtostrf(mlx->emissivity_, 10, 3, buf);
  while (*p == ' ') p++; // remove leading space
  send_answer_chunk(channel_mask, p, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":TR=", 0);
  p = my_dtostrf(mlx->t_room_, 10, 1, buf);
  while (*p == ' ') p++; // remove leading space
  send_answer_chunk(channel_mask, p, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RR=", 0);
  uint8_to_hex(buf, rr);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":RES=", 0);
  uint8_to_hex(buf, res);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":MODE=", 0);
  uint8_to_hex(buf, mode);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":FLAGS=", 0);
  uint8_to_hex(buf, mlx->flags_);
  send_answer_chunk(channel_mask, buf, 0);

  uint8_t is_first_flag = 1;

  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_BROKEN_PIXELS))
  {
    if (is_first_flag)
    {
      is_first_flag = 0;
      send_answer_chunk(channel_mask, "(", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "CORRECT_BROKEN_PIXELS", 0);
  }
  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_OUTLIER_PIXELS))
  {
    if (is_first_flag)
    {
      is_first_flag = 0;
      send_answer_chunk(channel_mask, "(", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "CORRECT_OUTLIER_PIXELS", 0);
  }
  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_DEINTERLACE_FILTER))
  {
    if (is_first_flag)
    {
      is_first_flag = 0;
      send_answer_chunk(channel_mask, "(", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "DEINTERLACE_FILTER", 0);
  }
  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_IIR_FILTER))
  {
    if (is_first_flag)
    {
      is_first_flag = 0;
      send_answer_chunk(channel_mask, "(", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "IIR_FILTER", 0);
  }
  if (mlx->flags_ & (1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME))
  {
    if (is_first_flag)
    {
      is_first_flag = 0;
      send_answer_chunk(channel_mask, "(", 0);
    } else
    {
      send_answer_chunk(channel_mask, ",", 0);
    }
    send_answer_chunk(channel_mask, "ON_FULL_FRAME", 0);
  }

  send_answer_chunk(channel_mask, (is_first_flag == 0) ? ")" : "", 1);

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
  send_answer_chunk(channel_mask, ":RO:MV_RES=" xstr(MLX90640_LSB_C) "," xstr(MLX90640_LSB_C) "[768]", 1);
}


void
cmd_90640_cs_write(uint8_t sa, uint8_t channel_mask, const char *input)
{//emissivity - TR - RR - RES - MODE - FLAGS
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  char buf[16]; memset(buf, 0, sizeof(buf));
  if (mlx == NULL)
  {
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }
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
    float t_room = atof(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if ((t_room >= -60) && (t_room <= 100))
    {
      mlx->t_room_ = t_room;
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
    if ((rr >= 0) && (rr <= 7))
    {
      int ret = MLX90640_SetRefreshRate(sa, rr);
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
    if ((res >= 0) && (res <= 7))
    {
      MLX90640_SetResolution(sa, res);
      send_answer_chunk(channel_mask, ":RES=OK [mlx-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":RES=FAIL; outbound", 1);
    }
    return;
  }
  var_name = "MODE=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if (!strcmp(input+strlen(var_name), "CHESS"))
    {
      MLX90640_SetChessMode(sa);
      send_answer_chunk(channel_mask, ":MODE=OK [mlx-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "TV"))
    {
      MLX90640_SetInterleavedMode(sa);
      send_answer_chunk(channel_mask, ":MODE=OK [mlx-register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":MODE=FAIL; outbound", 1);
    }
    return;
  }
  var_name = "FLAGS=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    if (!strcmp(input+strlen(var_name), "+BROKEN"))
    {
      mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_BROKEN_PIXELS);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "-BROKEN"))
    {
      mlx->flags_ &= ~(1U<<MLX90640_CMD_FLAG_BROKEN_PIXELS);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "+OUTLIER"))
    {
      mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_OUTLIER_PIXELS);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "-OUTLIER"))
    {
      mlx->flags_ &= ~(1U<<MLX90640_CMD_FLAG_OUTLIER_PIXELS);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "+DEINTERLACE"))
    {
      mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_DEINTERLACE_FILTER);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "-DEINTERLACE"))
    {
      mlx->flags_ &= ~(1U<<MLX90640_CMD_FLAG_DEINTERLACE_FILTER);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "+IIR"))
    {
      mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_IIR_FILTER);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
      if (mlx->iir_ != NULL)
      { // this will 'reset' the IIR buffer!
        free(mlx->iir_);
        mlx->iir_ = NULL;
      }
    }
    else if (!strcmp(input+strlen(var_name), "-IIR"))
    {
      mlx->flags_ &= ~(1U<<MLX90640_CMD_FLAG_IIR_FILTER);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
      if (mlx->iir_ != NULL)
      { // clean up the IIR buffer
        free(mlx->iir_);
        mlx->iir_ = NULL;
      }
    }
    else if (!strcmp(input+strlen(var_name), "+FF"))
    {
      mlx->flags_ |= (1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else if (!strcmp(input+strlen(var_name), "-FF"))
    {
      mlx->flags_ &= ~(1U<<MLX90640_CMD_FLAG_ND_ON_FULL_FRAME);
      send_answer_chunk(channel_mask, ":FLAGS=OK [hub-register]", 1);
    }
    else
    {
      send_answer_chunk(channel_mask, ":FLAGS=FAIL; unknown value '", 0);
      send_answer_chunk(channel_mask, input+strlen(var_name), 0);
      send_answer_chunk(channel_mask, "'", 1);
    }
    return;
  }
  var_name = "SA=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t new_sa = atohex8(input+strlen(var_name));
    send_answer_chunk(channel_mask, "+cs:", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);

    if (new_sa == sa)
    {
      send_answer_chunk(channel_mask, ":SA=same; not updated", 1);
      return;
    }

    if (hal_i2c_slave_address_available(new_sa))
    {
      send_answer_chunk(channel_mask, ":SA=FAIL '", 0);
      uint8_to_hex(buf, new_sa);
      send_answer_chunk(channel_mask, buf, 0);
      send_answer_chunk(channel_mask, "' is in use; not updated", 1);
      return;
    }

    if ((new_sa >= 3) && (new_sa <= 126))
    { // trick to update the SA (Slave Address) in the EEPROM
      uint16_t value = 0;
      int16_t error = MLX90640_I2CRead(sa, 0x240F, 1, &value);
      value &= 0xFF00; // keep MSB
      value |= new_sa; // use new_sa as LSB (LSB = slave address)

      if (error == 0)
      {
        MLX90640_I2CWrite(sa, 0x240F, 0x0000); // erase EE
        MLX90640_I2CWrite(sa, 0x240F, value); // write to EE
        MLX90640_I2CWrite(sa, 0x8010, value); // write to RAM (new_sa is now active!)
      }

      // keep same slave active after adress update.
      if (g_active_slave == sa)
      {
        g_active_slave = new_sa;
      }

      // disconnect old-SA.
      cmd_90640_tear_down(sa);
      g_sa_list[sa].found_ = 0; // reset 'found' bit on 'old'-SA.

      // we assume that the new SA will use the same driver!
      i2c_stick_register_driver(new_sa, DRV_MLX90640_ID);
      g_sa_list[new_sa].spot_ = 1; // set 'found' bit on 'new'-SA.

      send_answer_chunk(channel_mask, ":SA=OK! [mlx-EE]", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":SA=FAIL; outbound", 1);
    }
    return;
  }

  send_answer_chunk(channel_mask, "+cs:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":FAIL; unknown variable", 1);
}


void
cmd_90640_mr(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }

  uint8_t read_in_eeprom = true;
  if (mem_start_address >= (0x2400 + 832))
  { // read starts after EEPROM segment
    read_in_eeprom = false;
  }
  if ((mem_start_address + mem_count) < 0x2400)
  { // read ends before EEPROM segment
    read_in_eeprom = false;
  }

  // EEPROM can be read only at 400kHz I2C clock frequency
  // so if > 400k , limit to 400k, and restore original clock after
  uint16_t i2c_clock_enum = 0;
  if (read_in_eeprom)
  {
    i2c_clock_enum = i2c_stick_get_i2c_clock_frequency();
    if (i2c_clock_enum == HOST_CFG_I2C_F1M)
    {
      i2c_stick_set_i2c_clock_frequency(HOST_CFG_I2C_F400k);
    }
  }

  *bit_per_address = 16;
  *address_increments = 1;
  int result = MLX90640_I2CRead(sa, mem_start_address, mem_count, mem_data);

  if (read_in_eeprom)
  { // restore original I2C clock frequency
    i2c_stick_set_i2c_clock_frequency(i2c_clock_enum);
  }

  if (result < 0)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90640_ERROR_COMMUNICATION;
    return;
  }
}


void
cmd_90640_mw(uint8_t sa, uint16_t *mem_data, uint16_t mem_start_address, uint16_t mem_count, uint8_t *bit_per_address, uint8_t *address_increments, char const **error_message)
{
  MLX90640_t *mlx = cmd_90640_get_handle(sa);
  if (mlx == NULL)
  {
    *bit_per_address = 0;
    *address_increments = 0;
    *error_message = MLX90640_ERROR_NO_FREE_HANDLE;
    return;
  }
  if (mlx->slave_address_ & 0x80)
  {
    cmd_90640_init(sa);
  }

  uint8_t write_in_eeprom = true;
  if (mem_start_address >= (0x2400 + 832))
  { // read starts after EEPROM segment
    write_in_eeprom = false;
  }
  if ((mem_start_address + mem_count) < 0x2400)
  { // read ends before EEPROM segment
    write_in_eeprom = false;
  }

  // EEPROM can be written only at 400kHz I2C clock frequency
  // so if > 400k , limit to 400k, and restore original clock after
  uint16_t i2c_clock_enum = 0;
  if (write_in_eeprom)
  {
    i2c_clock_enum = i2c_stick_get_i2c_clock_frequency();
    if (i2c_clock_enum == HOST_CFG_I2C_F1M)
    {
      i2c_stick_set_i2c_clock_frequency(HOST_CFG_I2C_F400k);
    }
  }

  *bit_per_address = 16;
  *address_increments = 1;

  for (uint16_t i=0; i<mem_count; i++)
  {
    uint16_t addr = mem_start_address+i;
    if ((0x2400 <= addr) && (addr < (0x2400+832)))
    { // this is an EEPROM address...
      // ==> write zero before writing the data!
      if (mem_data[i] != 0)
      {
        MLX90640_I2CWrite(sa, addr, 0);
      }
    }
    int result = MLX90640_I2CWrite(sa, addr, mem_data[i]);

    if (result < 0)
    {
      if (write_in_eeprom)
      { // restore original I2C clock frequency
        i2c_stick_set_i2c_clock_frequency(i2c_clock_enum);
      }
      *bit_per_address = 0;
      *address_increments = 0;
      *error_message = MLX90640_ERROR_COMMUNICATION;
      return;
    }
  }

  if (write_in_eeprom)
  { // restore original I2C clock frequency
    i2c_stick_set_i2c_clock_frequency(i2c_clock_enum);
  }
}


void
cmd_90640_is(uint8_t sa, uint8_t *is_ok, char const **error_message)
{ // function to call prior any init, only to check is the connected slave IS a MLX90640.
  uint16_t value;
  *is_ok = 1; // be optimistic!

  MLX90640_I2CInit();
  if (MLX90640_I2CRead(sa, 0x8010, 1, &value) < 0)
  {
    *error_message = MLX90640_ERROR_COMMUNICATION;
    *is_ok = 0;
    return;
  }
  if ((value & 0x007F) != sa)
  {
    *is_ok = 0;
  }
  if (MLX90640_I2CRead(sa, 0x240A, 1, &value) < 0)
  {
    *error_message = MLX90640_ERROR_COMMUNICATION;
    *is_ok = 0;
    return;
  }
  if (value & 0x0040)
  {
    *is_ok = 0;
  }
}


void cmd_90640_iir_filter(float *to_list, float *iir, uint8_t depth, float threshold)
{
  for (uint16_t pix=0; pix<768; pix++)
  {
    if ((-100 <= to_list[pix]) && (to_list[pix] <= 1000))
    {
      if (fabs(to_list[pix] - iir[pix]) >= threshold)
      {
        iir[pix] += ((to_list[pix] - iir[pix]) * 0.9f); // take 90% of the change.
      } else
      {
        iir[pix] = (to_list[pix] + iir[pix] * (depth - 1)) / depth;
      }
    }
  }
}


static float helper_median(float *arr, uint16_t n)
{
  float temp = 0.0f;
  uint16_t min_idx;

  /* sorting */
  for (uint16_t i = 0; i < n - 1; i++)
  {
    // Find the minimum element in unsorted array
    min_idx = i;
    for (uint16_t j = i + 1; j < n; j++)
    {
      if (arr[j] < arr[min_idx])
      {
        min_idx = j;
      }
    }

    // Swap the found minimum element with the first element
    temp = arr[i];
    arr[i] = arr[min_idx];
    arr[min_idx] = temp;
  }

  /* get median */
  if((n % 2) == 0) // when even array length => average of 2 middle elements.
    return (arr[(n-1)/2] + arr[n/2])/2.0;

  return arr[n/2];
}


void cmd_90640_deinterlace_filter(float *to_list, uint8_t subpage)
{
  uint8_t odd = 0;
  float a = 2.0f;
  float temp_arr[6];

  if (subpage == 1)
  {
    for (uint16_t row=0; row<24; row++)
    {
      for (uint16_t col=odd; col<32; col+=2)
      {
        if (row == 0)
        {
          if (col == 0)
          {
            temp_arr[0] = to_list[(row+1)*32+(col+0)];
            temp_arr[1] = to_list[(row+0)*32+(col+1)];
            temp_arr[2] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 3);
          } else
          {
            temp_arr[0] = to_list[(row+0)*32+(col-1)];
            temp_arr[1] = to_list[(row+1)*32+(col+0)];
            temp_arr[2] = to_list[(row+0)*32+(col+1)];
            temp_arr[3] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 4);
          }
        } else if (row == 23)
        {
          if (col == 31)
          {
            temp_arr[0] = to_list[(row-1)*32+(col+0)];
            temp_arr[1] = to_list[(row+0)*32+(col-1)];
            temp_arr[2] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 3);
          } else
          {
            temp_arr[0] = to_list[(row+0)*32+(col-1)];
            temp_arr[1] = to_list[(row-1)*32+(col+0)];
            temp_arr[2] = to_list[(row+0)*32+(col+1)];
            temp_arr[3] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 4);
          }
        } else if ((col == 0) && ((row % 2) == 0))
        {
          temp_arr[0] = to_list[(row-1)*32+(col+0)];
          temp_arr[1] = to_list[(row+1)*32+(col+0)];
          temp_arr[2] = to_list[(row+0)*32+(col+1)];
          temp_arr[3] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 4);
        } else if ((col == 31) && ((row % 2) == 1))
        {
          temp_arr[0] = to_list[(row+0)*32+(col-1)];
          temp_arr[1] = to_list[(row-1)*32+(col+0)];
          temp_arr[2] = to_list[(row+1)*32+(col+0)];
          temp_arr[3] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 4);
        } else
        {
          temp_arr[0] = to_list[(row-1)*32+(col+0)];
          temp_arr[1] = to_list[(row+1)*32+(col+0)];
          temp_arr[2] = to_list[(row+0)*32+(col-1)];
          temp_arr[3] = to_list[(row+0)*32+(col+1)];
          temp_arr[4] = to_list[(row+0)*32+(col+0)];
          temp_arr[5] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 6);
        }
        if (fabs(a - to_list[row*32+col]) > 0.7)
        {
          to_list[row*32+col] = a;
        }
      }
      odd ^= 0x01;
    }
  } else
  {
    odd = 1;
    for (uint16_t row=0; row<24; row++)
    {
      for (uint16_t col=odd; col<32; col+=2)
      {
        if (row == 0)
        {
          if (col == 31)
          {
            temp_arr[0] = to_list[(row+0)*32+(col-1)];
            temp_arr[1] = to_list[(row+1)*32+(col+0)];
            temp_arr[2] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 3);
          } else
          {
            temp_arr[0] = to_list[(row+0)*32+(col-1)];
            temp_arr[1] = to_list[(row+1)*32+(col+0)];
            temp_arr[2] = to_list[(row+0)*32+(col+1)];
            temp_arr[3] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 4);
          }
        } else if (row == 23)
        {
          if (col == 0)
          {
            temp_arr[0] = to_list[(row-1)*32+(col+0)];
            temp_arr[1] = to_list[(row+0)*32+(col+1)];
            temp_arr[2] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 3);
          } else
          {
            temp_arr[0] = to_list[(row+0)*32+(col-1)];
            temp_arr[1] = to_list[(row-1)*32+(col+0)];
            temp_arr[2] = to_list[(row+0)*32+(col+1)];
            temp_arr[3] = to_list[(row+0)*32+(col+0)];
            a = helper_median(temp_arr, 4);
          }
        } else if ((col == 0) && ((row % 2) == 1))
        {
          temp_arr[0] = to_list[(row-1)*32+(col+0)];
          temp_arr[1] = to_list[(row+1)*32+(col+0)];
          temp_arr[2] = to_list[(row+0)*32+(col+1)];
          temp_arr[3] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 4);
        } else if ((col == 31) && ((row % 2) == 0))
        {
          temp_arr[0] = to_list[(row+0)*32+(col-1)];
          temp_arr[1] = to_list[(row-1)*32+(col+0)];
          temp_arr[2] = to_list[(row+1)*32+(col+0)];
          temp_arr[3] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 4);
        } else
        {
          temp_arr[0] = to_list[(row-1)*32+(col+0)];
          temp_arr[1] = to_list[(row+1)*32+(col+0)];
          temp_arr[2] = to_list[(row+0)*32+(col-1)];
          temp_arr[3] = to_list[(row+0)*32+(col+1)];
          temp_arr[4] = to_list[(row+0)*32+(col+0)];
          temp_arr[5] = to_list[(row+0)*32+(col+0)];
          a = helper_median(temp_arr, 6);
        }
        if (fabs(a - to_list[row*32+col]) > 0.7)
        {
          to_list[row*32+col] = a;
        }
      }
      odd ^= 0x01;
    }
  }
}

#ifdef __cplusplus
}
#endif

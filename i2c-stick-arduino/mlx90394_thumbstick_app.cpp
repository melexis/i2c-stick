#include "mlx90394_thumbstick_app.h"
#include "mlx90394_api.h"
#include "i2c_stick.h"
#include "i2c_stick_dispatcher.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_hal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <EEPROM.h> // EEPROM lib from Arduino


static uint8_t g_sa = 0x00;
static uint8_t g_we_disabled_sa = 0;

static int16_t cal_x_offset = 0;
static int16_t cal_y_offset = 0;

static int16_t last_x = 0;
static int16_t last_y = 0;

static int16_t cal_x_offset_EE = 0;
static int16_t cal_y_offset_EE = 2;



uint8_t
cmd_90394_thumbstick_app_begin(uint8_t channel_mask)
{
// configure the mlx09394 for the thumbstick app.
  uint8_t ok = 1;
  char buf[32];

  // find out which SA is a MLX90394
  if (g_sa == 0x00)
  {
		for (uint8_t sa = 1; sa<128; sa++)
		{
			send_answer_chunk(channel_mask, "==> app begin:", 0);
			sprintf(buf, "%02X", sa);
			send_answer_chunk(channel_mask, buf, 0);
			send_answer_chunk(channel_mask, " -- ", 0);
			sprintf(buf, "%d", g_sa_list[sa].found_);
			send_answer_chunk(channel_mask, buf, 1);
			if (g_sa_list[sa].found_)
			{
				char buf[16]; memset(buf, 0, sizeof(buf));
				int16_t spot = g_sa_list[sa].spot_;
				if (g_sa_drv_register[spot].drv_ == DRV_MLX90394_ID)
				{
					g_sa = sa;
					break;
				}
			}
		}
  }

  if (g_sa == 0x00) ok = 0;
  if (mlx90394_write_EN_X(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_Y(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_Z(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_T(g_sa) != 0) ok = 0;
  if (mlx90394_write_measurement_mode(g_sa, MLX90394_MODE_10Hz) != 0) ok = 0;

// temporary disable the EEPROM read function.
//  uint16_t byte = EEPROM.read(cal_x_offset_EE);
//  cal_x_offset = byte;
//  byte = EEPROM.read(cal_x_offset_EE+1);
//  cal_x_offset |= (byte << 8);
//
//  byte = EEPROM.read(cal_y_offset_EE);
//  cal_y_offset = byte;
//  byte = EEPROM.read(cal_y_offset_EE+1);
//  cal_y_offset |= (byte << 8);

  if (ok)
  {
    send_answer_chunk(channel_mask, ":", 0);
    itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":OK", 1);
  }
  else // when failed..
  {
    send_answer_chunk(channel_mask, ":", 0);
    itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":FAILED (app not started)", 1);
    return APP_NONE;
  }


// potentially disable the mlx90394 for emitting results in the continuous mode.
  g_we_disabled_sa = g_sa;


  return APP_MLX90394_THUMBSTICK_ID;
}


void
handle_90394_thumbstick_app(uint8_t channel_mask)
{
  static uint32_t prev_time = hal_get_millis();
  char buf[32]; memset(buf, 0, sizeof(buf));
  if (hal_get_millis() - prev_time > 100)
  {
    send_answer_chunk(channel_mask, "#", 0);
    itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    uint8_t ok = 1;
    int16_t x = 0x7FFF;
    int16_t y = 0x7FFF;
    int16_t z = 0x7FFF;
    int16_t t = 0x7FFF;
    float heading = 0;
    float deflect = 0;
    if (mlx90394_read_xyzt(g_sa, &x, &y, &z, &t) != 0) ok = 0;

    // here is the app calculation magic!
    if (z < 0)
    {
      x = -x;
      y = -y;
      z = -z;
    }
    last_x = x;
    last_y = y;

    x -= cal_x_offset;
    y -= cal_y_offset;


    heading = fmodf(atan2(x, y) + 2*M_PI, 2*M_PI)*180/M_PI;
    deflect = atan2(sqrt(x*x + y*y), z)*180/M_PI;

    // report the result
    send_answer_chunk(channel_mask, ":", 0);
		uint8_to_hex(buf, g_sa);
		send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ":", 0);
	  uint32_t time_stamp = hal_get_millis();
    uint32_to_dec(buf, time_stamp, 8);
		send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ":", 0);
    itoa(x, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    itoa(y, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    itoa(z, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    itoa(t, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    sprintf(buf, "%5.3f", heading);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    sprintf(buf, "%5.3f", deflect);
    send_answer_chunk(channel_mask, buf, 1);

    prev_time = hal_get_millis();
  }

}


uint8_t
cmd_90394_thumbstick_app_end(uint8_t channel_mask)
{
  char buf[32];
  send_answer_chunk(channel_mask, ":ENDING:", 0);
  itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);

  // re-enable when we did the disable at begin
  if (g_we_disabled_sa)
  {
    g_we_disabled_sa = 0;
  }
  return APP_NONE;
}


void
cmd_90394_thumbstick_ca(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":SA=", 0);
  uint8_to_hex(buf, g_sa);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":CAL_X=", 0);
  itoa(cal_x_offset, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":CAL_Y=", 0);
  itoa(cal_y_offset, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);
}


void
cmd_90394_thumbstick_ca_write(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "+ca:", 0);
  itoa(APP_MLX90394_THUMBSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);

  const char *var_name = "SA=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int16_t new_sa = atohex8(input+strlen(var_name));
    if ((new_sa >= 3) && (new_sa <= 126))
    { // only allow valid SA's
    	g_sa = new_sa;
      send_answer_chunk(channel_mask, ":SA=OK", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":SA=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "CMD=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    if (!strcasecmp(input+strlen(var_name), "NULL"))
    {
			cal_x_offset = last_x;
			cal_y_offset = last_y;

  		EEPROM.write(cal_x_offset_EE  , cal_x_offset & 0x00FF);
  		EEPROM.write(cal_x_offset_EE+1, ((cal_x_offset & 0xFF00) >> 8) & 0x00FF);
  		EEPROM.write(cal_y_offset_EE  , cal_y_offset & 0x00FF);
  		EEPROM.write(cal_y_offset_EE+1, ((cal_y_offset & 0xFF00) >> 8) & 0x00FF);
  	  EEPROM.commit();

			send_answer_chunk(channel_mask, ":CMD:NULL=OK", 1);
    }
    return;
  }

  var_name = "CAL_X=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int32_t new_value = atoi(input+strlen(var_name));
    if ((new_value >= -32768) && (new_value <= 32767))
    {
      cal_x_offset = new_value;
      EEPROM.write(cal_x_offset_EE  , cal_x_offset & 0x00FF);
      EEPROM.write(cal_x_offset_EE+1, ((cal_x_offset & 0xFF00) >> 8) & 0x00FF);
      EEPROM.commit();
      send_answer_chunk(channel_mask, ":CAL_X=OK", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":CAL_X=FAIL; outbound", 1);
    }
    return;
  }

  var_name = "CAL_Y=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    int32_t new_value = atoi(input+strlen(var_name));
    if ((new_value >= -32768) && (new_value <= 32767))
    {
      cal_y_offset = new_value;
      EEPROM.write(cal_y_offset_EE  , cal_y_offset & 0x00FF);
      EEPROM.write(cal_y_offset_EE+1, ((cal_y_offset & 0xFF00) >> 8) & 0x00FF);
      EEPROM.commit();
      send_answer_chunk(channel_mask, ":CAL_Y=OK", 1);
    } else
    {
      send_answer_chunk(channel_mask, ":CAL_Y=FAIL; outbound", 1);
    }
    return;
  }
}

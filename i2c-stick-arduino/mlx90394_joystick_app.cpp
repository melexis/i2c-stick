#include "mlx90394_joystick_app.h"
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

static int16_t cal_alpha_offset = 0;
static int16_t cal_beta_offset = 0;

static float last_alpha = 0;
static float last_beta = 0;

static int16_t cal_alpha_offset_EE = 0;
static int16_t cal_beta_offset_EE = 2;


uint8_t
cmd_90394_joystick_app_begin(uint8_t channel_mask)
{
// configure the mlx90394 for the joystick app.
  uint8_t ok = 1;
  char buf[32];

  // find out which SA is a MLX90394
  // find out which SA is a MLX90394
  g_sa = i2c_stick_get_current_slave_with_driver(DRV_MLX90394_ID);

  if (g_sa == 0x00) ok = 0;
  if (mlx90394_write_EN_X(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_Y(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_Z(g_sa) != 0) ok = 0;
  if (mlx90394_write_EN_T(g_sa) != 0) ok = 0;
  if (mlx90394_write_measurement_mode(g_sa, MLX90394_MODE_50Hz) != 0) ok = 0;

	uint16_t byte = EEPROM.read(cal_alpha_offset_EE);
	cal_alpha_offset = byte;
	byte = EEPROM.read(cal_alpha_offset_EE+1);
	cal_alpha_offset |= (byte << 8);

	byte = EEPROM.read(cal_beta_offset_EE);
	cal_beta_offset = byte;
	byte = EEPROM.read(cal_beta_offset_EE+1);
	cal_beta_offset |= (byte << 8);

  if (ok)
  {
    send_answer_chunk(channel_mask, ":", 0);
    itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":OK", 1);
  }
  else // when failed..
  {
    send_answer_chunk(channel_mask, ":", 0);
    itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    if (g_sa == 0x00)
    {
      send_answer_chunk(channel_mask, ":FAILED (no mlx90394 found, try scan, app not started)", 1);
    }
    else
    {
      send_answer_chunk(channel_mask, ":FAILED (communication error, app not started)", 1);
    }    
    return APP_NONE;
  }


// potentially disable the mlx90394 for emitting results in the continuous mode.
  g_we_disabled_sa = g_sa;

  return APP_MLX90394_JOYSTICK_ID;
}


void
handle_90394_joystick_app(uint8_t channel_mask)
{
  static uint32_t prev_time = hal_get_millis();
  char buf[32]; memset(buf, 0, sizeof(buf));
  if (hal_get_millis() - prev_time > 20)
  {
    send_answer_chunk(channel_mask, "#", 0);
    itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
    send_answer_chunk(channel_mask, buf, 0);
    uint8_t ok = 1;
    int16_t x = 0x7FFF;
    int16_t y = 0x7FFF;
    int16_t z = 0x7FFF;
    int16_t t = 0x7FFF;
    float alpha = 0;
    float beta = 0;
    if (mlx90394_read_xyzt(g_sa, &x, &y, &z, &t) != 0) ok = 0;

    // here is the app calculation magic!
    if (z < 0)
    {
      x = -x;
      y = -y;
      z = -z;
    }

    alpha = atan2(x, z)*180/M_PI;
    beta  = atan2(y, z)*180/M_PI;

    last_alpha = alpha;
    last_beta = beta;

    alpha -= (float(cal_alpha_offset)/1024);
    beta -= (float(cal_beta_offset)/1024);

		// scaling and clamping.
    alpha *= 50.0f/9.0f;
    beta *= 50.0f/9.0f;

    alpha += 50;
    beta += 50;

    if (alpha > 100) alpha = 100;
    if (alpha < 0) alpha = 0;
    if (beta > 100) beta = 100;
    if (beta < 0) beta = 0;

    // add a dead zone
    if ((48 < alpha) && (alpha < 52))
    {
    	if ((48 < beta ) && (beta  < 52))
    	{
    	  beta  = 50;
    	  alpha = 50;
    	}
    }

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
    sprintf(buf, "%5.3f", alpha);
    send_answer_chunk(channel_mask, buf, 0);

    send_answer_chunk(channel_mask, ",", 0);
    sprintf(buf, "%5.3f", beta);
    send_answer_chunk(channel_mask, buf, 1);

    prev_time = hal_get_millis();
  }
}


uint8_t
cmd_90394_joystick_app_end(uint8_t channel_mask)
{
  char buf[32];
  send_answer_chunk(channel_mask, ":ENDING:", 0);
  itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);

  // re-enable when we did the disable at begin
  if (g_we_disabled_sa)
  {
    g_we_disabled_sa = 0;
  }
  return APP_NONE;
}


void
cmd_90394_joystick_ca(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":SA=", 0);
  uint8_to_hex(buf, g_sa);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":CAL_X=", 0);
  itoa(cal_alpha_offset, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "ca:", 0);
  itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":CAL_Y=", 0);
  itoa(cal_beta_offset, buf, 10);
  send_answer_chunk(channel_mask, buf, 1);
}


void
cmd_90394_joystick_ca_write(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  send_answer_chunk(channel_mask, "+ca:", 0);
  itoa(APP_MLX90394_JOYSTICK_ID, buf, 10);
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
			cal_alpha_offset = (last_alpha * 1024);
			cal_beta_offset = (last_beta * 1024);

  		EEPROM.write(cal_alpha_offset_EE  , cal_alpha_offset & 0x00FF);
  		EEPROM.write(cal_alpha_offset_EE+1, ((cal_alpha_offset & 0xFF00) >> 8) & 0x00FF);
  		EEPROM.write(cal_beta_offset_EE  , cal_beta_offset & 0x00FF);
  		EEPROM.write(cal_beta_offset_EE+1, ((cal_beta_offset & 0xFF00) >> 8) & 0x00FF);
      // EEPROM.commit();

			send_answer_chunk(channel_mask, ":CMD:NULL=OK", 1);
    }
    return;
  }
}

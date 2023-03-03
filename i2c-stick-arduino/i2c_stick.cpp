#include "i2c_stick.h"

sa_list_t g_sa_list[128];
sa_drv_register_t g_sa_drv_register[MAX_SA_DRV_REGISTRATIONS];

static_assert(sizeof(sa_drv_register_t) == 2, "type 'sa_drv_register_t' should be two bytes!");
static_assert(sizeof(sa_list_t) == 1, "type 'sa_list_t' should be one byte!");
static_assert(MAX_SA_DRV_REGISTRATIONS <= (1ULL<< ((8 * sizeof(g_sa_list[0]))-1)), "MAX_SA_DRV_REGISTRATIONS is too large; Consider updating the type of the variable 'g_sa_list'");

// const char *handle_cmd(uint8_t channel_mask, const char *cmd);
// void send_broadcast_message(const char *msg);
// void send_answer_chunk(uint8_t channel_mask, const char *answer, uint8_t terminate);

static int16_t 
sa_drv_get_free_spot()
{
  for (uint16_t i=1; i<MAX_SA_DRV_REGISTRATIONS; i++)
  {
    if ((g_sa_drv_register[i].sa_ == 0) && (g_sa_drv_register[i].drv_ == 0))
    {
      return i;
    }
  }
  return -1;
}


static int16_t 
sa_drv_get_spot(uint8_t sa, uint8_t drv)
{
  for (uint16_t i=1; i<MAX_SA_DRV_REGISTRATIONS; i++)
  {
    if ((g_sa_drv_register[i].sa_ == sa) && (g_sa_drv_register[i].drv_ == drv))
    {
      return i;
    }
  }
  return -1;
}


int16_t
i2c_stick_register_driver(uint8_t sa, uint8_t drv)
{
  // sanity check
  if (sa > 127) return -2; // invalid I2C slave address.
  if (drv > 63) return -3; // invalid drv-id number.
  if ((sa == 0) && (drv == 0)) return -4; // I2C slave address and drv cannot be zero at the same time.

  int16_t spot = sa_drv_get_spot(sa, drv);
  if (spot < 0)
  { // does not exists yet.
    spot = sa_drv_get_free_spot();
    if (spot < 0) 
    { // no free spot anymore!
      return -1;
    }
  }
  g_sa_drv_register[spot].sa_ = sa;
  g_sa_drv_register[spot].drv_ = drv;
  g_sa_drv_register[spot].disabled_ = 0;
  g_sa_drv_register[spot].nd_ = 0;
  g_sa_drv_register[spot].raw_ = 0;
  return spot;
}



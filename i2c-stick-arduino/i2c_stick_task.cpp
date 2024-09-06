#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_task.h"
#include "i2c_stick_hal.h"
#include "i2c_stick_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *
handle_task(uint8_t channel_mask, char task)
{
  if (task == ';') // go-task
  {
    g_mode = MODE_CONTINUOUS;
    send_answer_chunk(channel_mask, ";:continuous mode", 1);
    return "";
  }
  if (task == '!') // halt-task
  {
    g_mode = MODE_INTERACTIVE;
    send_answer_chunk(channel_mask, "!:interactive mode", 1);
    return "";
  }
  if (task == '>') // next-sensor-task
  {
    handle_task_next(channel_mask);
    return "";
  }
  if (task == '<') // previous-sensor-task
  {
    uint8_t old_active_slave = g_active_slave;
    for (int8_t sa=old_active_slave-1; sa>=0; sa--)
    {
      int16_t spot = g_sa_list[sa].spot_;
      if ((g_sa_list[sa].found_) && ((g_sa_drv_register[spot].drv_) > 0))
      {
        g_active_slave = sa;
        break;
      }
    }
    if (g_active_slave == old_active_slave)
    {
      for (uint8_t sa=127; sa>old_active_slave; sa--)
      {
        int16_t spot = g_sa_list[sa].spot_;
        if ((g_sa_list[sa].found_) && ((g_sa_drv_register[spot].drv_) > 0))
        {
          g_active_slave = sa;
          break;
        }
      }
    }
    char buf[32]; memset(buf, 0, sizeof(buf));
    int16_t spot = g_sa_list[g_active_slave].spot_;
    strcpy(buf, "<:");
    strcpy(buf+strlen(buf), bytetohex(g_active_slave));
    strcpy(buf+strlen(buf), ":");
    strcpy(buf+strlen(buf), bytetostr(g_sa_drv_register[spot].drv_));
    strcpy(buf+strlen(buf), ",");

    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(g_sa_drv_register[spot].drv_), 1);
    return "";
  }
  if ((task == '?') || (task == '1')) // help-task
  {
    send_answer_chunk(channel_mask, "Usage: Melexis I2C STICK", 1);
    send_answer_chunk(channel_mask, "========================", 1);
    handle_task_help(channel_mask);
    g_mode = MODE_INTERACTIVE;
    return "";
  }
  if (task == '5') // refresh task ==> redirect to scan command.
  {
    handle_cmd(channel_mask, "scan");
    return "";
  }
  return NULL; // no task!
}


void
handle_task_next(uint8_t channel_mask)
{
  uint8_t old_active_slave = g_active_slave;
  for (uint8_t sa=old_active_slave+1; sa<128; sa++)
  {
    int16_t spot = g_sa_list[sa].spot_;
    if ((g_sa_list[sa].found_) && ((g_sa_drv_register[spot].drv_) > 0))
    {
      g_active_slave = sa;
      break;
    }
  }
  if (g_active_slave == old_active_slave)
  {
    for (uint8_t sa=0; sa<old_active_slave; sa++)
    {
      int16_t spot = g_sa_list[sa].spot_;
      if ((g_sa_list[sa].found_) && ((g_sa_drv_register[spot].drv_) > 0))
      {
        g_active_slave = sa;
        break;
      }
    }
  }
  char buf[32]; memset(buf, 0, sizeof(buf));
  int16_t spot = g_sa_list[g_active_slave].spot_;
  strcpy(buf, ">:");
  strcpy(buf+strlen(buf), bytetohex(g_active_slave));
  strcpy(buf+strlen(buf), ":");

  strcpy(buf+strlen(buf), bytetostr(g_sa_drv_register[spot].drv_));
  strcpy(buf+strlen(buf), ",");

  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(g_sa_drv_register[spot].drv_), 1);
}


void
handle_task_help(uint8_t channel_mask)
{
  send_answer_chunk(channel_mask, "Tasks: single character tasks; no new line required", 1);
  send_answer_chunk(channel_mask, "- ;  ==>  enter continuous mode", 1);
  send_answer_chunk(channel_mask, "- !  ==>  quit continuous mode", 1);
  send_answer_chunk(channel_mask, "- >  ==>  next active slave", 1);
  send_answer_chunk(channel_mask, "- <  ==>  previous active slave", 1);
  send_answer_chunk(channel_mask, "- ?  ==>  this help", 1);
  send_answer_chunk(channel_mask, "- 1  ==>  this help", 1);
  send_answer_chunk(channel_mask, "- 5  ==>  scan command alias", 1);
  send_answer_chunk(channel_mask, "", 1);
  send_answer_chunk(channel_mask, "Commands: 2+ character commands with new line required", 1);
  send_answer_chunk(channel_mask, "- mlx  ==>  test uplink communication", 1);
  send_answer_chunk(channel_mask, "- help ==>  this help!", 1);
  send_answer_chunk(channel_mask, "- sos  ==>  more detailed help!", 1);
  send_answer_chunk(channel_mask, "- fv   ==>  Firmware Version", 1);
  send_answer_chunk(channel_mask, "- bi   ==>  Board Info", 1);
  send_answer_chunk(channel_mask, "- scan ==>  SCAN I2C bus for slaves", 1);
  send_answer_chunk(channel_mask, "- ls   ==>  List Slaves (already discovered with 'scan')", 1);
  send_answer_chunk(channel_mask, "- dis  ==>  DIsable Slave (for continuous dump mode)", 1);
  send_answer_chunk(channel_mask, "- i2c  ==>  low level I2C", 1);
  send_answer_chunk(channel_mask, "- ch   ==>  Configuration of Host (I2C freq, output format)", 1);  
#ifdef BUFFER_COMMAND_ENABLE
  send_answer_chunk(channel_mask, "- buf  ==>  buffer command", 1);
#endif // BUFFER_COMMAND_ENABLE
  send_answer_chunk(channel_mask, "", 1);
  send_answer_chunk(channel_mask, "- as   ==>  Active Slave", 1);
  send_answer_chunk(channel_mask, "- mv   ==>  Measure Value of the sensor", 1);
  send_answer_chunk(channel_mask, "- sn   ==>  Serial Number of slave", 1);
  send_answer_chunk(channel_mask, "- cs   ==>  Configuration of Slave", 1);
  send_answer_chunk(channel_mask, "- nd   ==>  New Data available", 1);
  send_answer_chunk(channel_mask, "- mr   ==>  Memory Read", 1);
  send_answer_chunk(channel_mask, "- mw   ==>  Memory Write", 1);
  send_answer_chunk(channel_mask, "- raw  ==>  RAW sensor data dump", 1);
  send_answer_chunk(channel_mask, "", 1);
  send_answer_chunk(channel_mask, "- la   ==>  List Applications", 1);
  send_answer_chunk(channel_mask, "- app  ==>  APPlication id", 1);
  send_answer_chunk(channel_mask, "- ca   ==>  Configuration of Application", 1);
  send_answer_chunk(channel_mask, "", 1);
  send_answer_chunk(channel_mask, "more at https://github.com/melexis/i2c-stick", 1);
}




#ifdef __cplusplus
}
#endif

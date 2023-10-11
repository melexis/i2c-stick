#include "i2c_stick.h"
#include "i2c_stick_cmd.h"
#include "i2c_stick_task.h"
#include "i2c_stick_hal.h"
#include "i2c_stick_dispatcher.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// arduino should not be here, it is only to ease debugging
// #include <Arduino.h>


#ifdef __cplusplus
extern "C" {
#endif


// supporting functions

char nibble_to_hex(uint8_t nibble) {  // convert a 4-bit nibble to a hexadecimal character
  nibble &= 0xF;
  return nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
}

void
uint8_to_hex(char *hex, uint8_t dec)
{
  hex[2] = '\0';
  hex[1] = nibble_to_hex(dec); dec >>= 4;
  hex[0] = nibble_to_hex(dec);
}


void
uint16_to_hex(char *hex, uint16_t dec)
{
  hex[4] = '\0';
  hex[3] = nibble_to_hex(dec); dec >>= 4;
  hex[2] = nibble_to_hex(dec); dec >>= 4;
  hex[1] = nibble_to_hex(dec); dec >>= 4;
  hex[0] = nibble_to_hex(dec);
}


void
uint32_to_dec(char *str, uint32_t dec, int8_t digits)
{
  str[digits] = '\0';
  if (digits <= 0) return;
  for (int8_t d=digits-1; d>=0; d--)
  {
    str[d] = '0' + (dec % 10);
    dec /= 10;
  }
}


int16_t atohex8(const char *in)
{
   uint8_t c, h;

   c = in[0];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   h = c;

   c = in[1];

   if (c <= '9' && c >= '0') {  c -= '0'; }
   else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
   else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
   else return(-1);

   return ( h<<4 | c);
}


int32_t atohex16(const char *in)
{
  uint8_t c;
  uint16_t h = 0;

  for (uint8_t i=0; ; i++)
  {
    c = in[i];
    if (c <= '9' && c >= '0') {  c -= '0'; }
    else if (c <= 'f' && c >= 'a') { c -= ('a' - 0x0a); }
    else if (c <= 'F' && c >= 'A') { c -= ('A' - 0x0a); }
    else return(-1);
    h |= c;
    if (i >= 3) break;
    h <<= 4;
  }

  return int32_t(h);
}


const char *
bytetohex(uint8_t dec)
{
  static char hex[3];
  uint8_t c = dec / 16;
  if (c < 10)
  {
    hex[0] = '0' + c;
  } else
  {
    hex[0] = 'A' + c - 10;
  }
  c = dec % 16;
  if (c < 10)
  {
    hex[1] = '0' + c;
  } else
  {
    hex[1] = 'A' + c - 10;
  }
  return hex;
}


const char *
bytetostr(uint8_t dec)
{
  static char str[4];
  memset(str, 0, sizeof(str));
  uint8_t c = dec / 100;
  uint8_t i = 0;

  str[i] = '0' + c;
  if (c > 0) i++;

  c = (dec / 10) % 10;
  str[i] = '0' + c;
  if (c > 0) i++;

  c = dec % 10;
  str[i] = '0' + c;
  return str;
}

// https://github.com/pmalasp/dtostrf/blob/master/dtostrf.c
char *my_dtostrf( float val,  int8_t char_num, uint8_t precision, char *chr_buffer)
{
  int       right_j;
  int       i, j ;
  float     r_val;
  long      i_val;
  char      c, c_sign;


  // check the sign
  if (val < 0.0) {
    // print the - sign
    c_sign = '-';

    // process the absolute value
    val = - val;
  } else {
    // put a space for positive numbers
    c_sign = ' ';

  }

  // check the left-right justification
  if (char_num < 0)
  {
    // set the flag
    right_j = 1;

    // make the number positive
    char_num = -char_num;

  } else {
    right_j = 0;
  }


  // no native exponential function for int
  j=1;
  for(i=0; i < (char_num - precision - 3 );i++) j *= 10;

  // Hackish fail-fast behavior for larger-than-what-can-be-printed values, countig the precision + sign ('-') +'.' + '\0'
  if (val >= (float)(j))
  {
    // not enough space
    // strcpy(chr_buffer, "ovf"); - this is very byte consuming (388 bytes) , so we go for the cheap array
    chr_buffer[0] = 'o';
    chr_buffer[1] = 'v';
    chr_buffer[2] = 'f';
    chr_buffer[3] = '\0';

    // finish here
    return chr_buffer;
  }



  // Simplistic rounding strategy so that e.g. print(1.999, 2)
  // prints as "2.00"
  r_val = 0.5;
  for ( i = 0; i < precision; i++) {
      r_val /= 10.0;
  }
  val += r_val;

  // Extract the integer and decimal part of the number and print it
  i_val = (long) val;
  r_val = val - (float) i_val;


  // print the integral part ... but it is in reverse order ... so leaves the space for  '.' and the decimal part (and remember that array indexes start from 0
  i = char_num - precision - 2;
  do
  {
      chr_buffer[i] = '0' + ( i_val % 10);
      i_val /= 10;
      i--;

  }   while ( i_val > 0) ;

  // add the sign char
  chr_buffer[i] = c_sign;

  // prepare for the decimal part
  j = char_num - precision - 1;

  // Print the decimal point, but only if there are digits beyond
  if (precision > 0) {
    chr_buffer[j] = '.';
    j++;

    // Extract digits from the remainder one at a time
    while (precision > 0) {
      // prepare the data
      r_val *= 10.0;
      i_val  = (int)    r_val;
      r_val -= (float)  i_val;

      // update the string
      chr_buffer[j] = '0' + ( i_val );
      j++;

      // use precision as the counter
      precision--;
    }
  }

  // terminate the string
  chr_buffer[j] = '\0';

  // check the justification direction
  if (right_j)
  {
    // pad the string with leading ' '
    while (i > 0)
    {
      i--;

      chr_buffer[i] = ' ';
    }

  }

 // return the pointer to the first char of the prepared string
 return ( &chr_buffer[i]);
}


// end supporting functions



const char *
handle_cmd(uint8_t channel_mask, const char *cmd)
{
  const char *this_cmd;

  this_cmd = "mlx"; // MeLeXis test command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    send_answer_chunk(channel_mask, "mlx:MELEXIS I2C STICK", 1);
    send_answer_chunk(channel_mask, "mlx:=================", 1);
    send_answer_chunk(channel_mask, "mlx:", 1);
    send_answer_chunk(channel_mask, "mlx:Melexis Inspired Engineering", 1);
    send_answer_chunk(channel_mask, "mlx:", 1);
    send_answer_chunk(channel_mask, "mlx:hit '?' for help", 1);
    return NULL;
  }

  this_cmd = "fv"; // get Firmware Version
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    char buf[16]; memset(buf, 0, sizeof(buf));
    send_answer_chunk(channel_mask, "fv:" FW_VERSION, 1);
    return NULL;
  }

  this_cmd = "bi"; // Board Information command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    char buf[64]; memset(buf, 0, sizeof(buf));
    send_answer_chunk(channel_mask, "bi:", 0);
    send_answer_chunk(channel_mask, hal_get_board_info(), 1);
    return NULL;
  }

  this_cmd = "ch"; // Config Hub command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    handle_cmd_ch(channel_mask, cmd+strlen(this_cmd));
    return NULL;
  }

  this_cmd = "+ch:"; // Config Hub command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    handle_cmd_ch_write(channel_mask, cmd+strlen(this_cmd));
    return NULL;
  }

  this_cmd = "help"; // help command, same as task ?
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    return handle_task(channel_mask, '?');
  }

  this_cmd = "sos"; // SOS command, get more help on a specific command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    handle_cmd_sos(channel_mask, cmd+strlen(this_cmd));
    return NULL;
  }

#ifdef BUFFER_COMMAND_ENABLE
  this_cmd = "buf"; // buffer command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    send_answer_chunk(channel_mask, g_channel_buffer, 1);
    return NULL;
  }
#endif // BUFFER_COMMAND_ENABLE

  this_cmd = "i2c:"; // raw I2C command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    char buf[128]; memset(buf, 0, sizeof(buf));
    send_answer_chunk(channel_mask, this_cmd, 0);

    int16_t sa = atohex8(cmd+strlen(this_cmd));
    if ((sa < 0) || (sa >= 0x80) || (cmd[6] != ':'))
    {
      send_answer_chunk(channel_mask, "ERROR:syntax error in slave address", 1);
      return NULL;
    }

    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":", 0);

    if (cmd[7] == 'R')
    {
      // example: read 1 byte from slave address 0x33 (sa is 7bit)
      // cmd: i2c:33:R1
      // cmd: 012345678
      send_answer_chunk(channel_mask, "R:", 0);
      uint16_t read_n_bytes = atoi(&cmd[8]);

      uint8_t read_buffer[64]; memset(read_buffer, 0, sizeof(read_buffer));

      int16_t result = hal_i2c_direct_read(sa, read_buffer, read_n_bytes);
      for (uint8_t i=0; i<read_n_bytes; i++)
      {
        uint8_to_hex(buf, read_buffer[i]);
        send_answer_chunk(channel_mask, buf, 0);
      }

      if (result == 0) // SUCCESS
      {
        send_answer_chunk(channel_mask, ":OK", 1);
      } else
      {
        send_answer_chunk(channel_mask, ":FAIL:", 0);
        uint8_to_hex(buf, result);
        send_answer_chunk(channel_mask, buf, 1);
      }
    } else if (cmd[7] == 'W')
    {
      // check if a read follows => repeated start => indirect read functions
      if (strchr(&cmd[7], 'R'))
      { // 'R' is found
        uint8_t read_buffer[64]; memset(read_buffer, 0, sizeof(read_buffer));
        uint8_t write_buffer[8]; memset(write_buffer, 0, sizeof(write_buffer));
        uint8_t i=0;
        for(; i<8; i++)
        {
          int16_t data = atohex8(cmd+8+i*2);
          if (data < 0) break;
          write_buffer[i] = data;
          uint8_to_hex(buf, write_buffer[i]);
          send_answer_chunk(channel_mask, buf, 0);
        }
        uint16_t read_n_bytes = atoi(&cmd[8+i*2+1]);
        int16_t result = hal_i2c_indirect_read(sa, write_buffer, i, read_buffer, read_n_bytes);
        if (result == 0) // SUCCESS
        {
          send_answer_chunk(channel_mask, ":R:", 0);
          for (uint8_t i=0; i<read_n_bytes; i++)
          {
            uint8_t data = read_buffer[i];
            uint8_to_hex(buf, data);
            send_answer_chunk(channel_mask, buf, 0);
          }
          send_answer_chunk(channel_mask, ":OK", 1);
        } else
        {
          send_answer_chunk(channel_mask, ":FAIL:", 0);
          uint8_to_hex(buf, result);
          send_answer_chunk(channel_mask, buf, 1);
        }
      } else
      { // no 'R' => direct write
        uint8_t write_buffer[64]; memset(write_buffer, 0, sizeof(write_buffer));
        uint8_t i=0;
        for(; i<64; i++)
        {
          int16_t data = atohex8(cmd+8+i*2);
          if (data < 0) break;
          write_buffer[i] = data;
          uint8_to_hex(buf, write_buffer[i]);
          send_answer_chunk(channel_mask, buf, 0);
        }
        int16_t result = hal_i2c_direct_write(sa, write_buffer, i);
        if (result == 0) // SUCCESS
        {
          send_answer_chunk(channel_mask, ":OK", 1);
        } else
        {
          send_answer_chunk(channel_mask, ":FAIL:", 0);
          uint8_to_hex(buf, result);
          send_answer_chunk(channel_mask, buf, 1);
        }
      }
    } else
    {
      send_answer_chunk(channel_mask, "ERROR:No R|W info found in cmd", 1);
      return NULL;
    }

    return NULL;
  }

  this_cmd = "scan"; // SCAN i2c bus command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    cmd_tear_down(255); // tear down all current drivers; if any.

    uint8_t count_slaves = 0;

    for (int16_t spot = 0; spot<MAX_SA_DRV_REGISTRATIONS; spot++)
    {
      g_sa_drv_register[spot].raw_ = 0;
      g_sa_drv_register[spot].disabled_ = 0;
      g_sa_drv_register[spot].nd_ = 0;
    }

    // SCL low for >1.44ms ==> MLX90614 in PWM or thermal relay requires this to enter communication mode
    hal_write_pin(17u, 0);
    hal_delay(2);
    hal_write_pin(17u, 1);

    for (uint8_t sa = 1; sa<128; sa++)
    {
      g_sa_list[sa].found_ = 0;
      if (hal_i2c_slave_address_available(sa))
      {
        const char *error_message;
        char buf[16]; memset(buf, 0, sizeof(buf));
        uint8_t is_ok = 0;
        uint8_t is_known_driver = 0;

        count_slaves++;
        g_sa_list[sa].spot_ = 0;
        g_sa_list[sa].found_ = 1;

        for (uint16_t spot=1; spot<MAX_SA_DRV_REGISTRATIONS; spot++)
        {
          if ((g_sa_drv_register[spot].sa_ == sa))
          {
            uint8_t drv = g_sa_drv_register[spot].drv_;
            cmd_is(sa, drv, &is_ok, &error_message);

            if (!is_ok)
            {
              continue;
            }
            is_known_driver = 1;
            // ok we are good to go!
            send_answer_chunk(channel_mask, this_cmd, 0);
            send_answer_chunk(channel_mask, ":", 0);
            uint8_to_hex(buf, sa);
            send_answer_chunk(channel_mask, buf, 0);
            send_answer_chunk(channel_mask, ":", 0);
            uint8_to_hex(buf, g_sa_drv_register[spot].drv_);
            send_answer_chunk(channel_mask, buf, 0);
            send_answer_chunk(channel_mask, ",", 0);
            uint8_to_hex(buf, g_sa_drv_register[spot].raw_);
            send_answer_chunk(channel_mask, buf, 0);
            send_answer_chunk(channel_mask, ",", 0);
            uint8_to_hex(buf, g_sa_drv_register[spot].disabled_);
            send_answer_chunk(channel_mask, buf, 0);
            send_answer_chunk(channel_mask, ",", 0);

            send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(g_sa_drv_register[spot].drv_), 1);
            g_sa_list[sa].spot_ = spot;
            break;
          }
        }
        if (!is_known_driver)
        {
          send_answer_chunk(channel_mask, this_cmd, 0);
          send_answer_chunk(channel_mask, ":", 0);
          uint8_to_hex(buf, sa);
          send_answer_chunk(channel_mask, buf, 0);
          send_answer_chunk(channel_mask, ":00,00,00,Unknown", 1);
        }
      }
    }

    if (count_slaves == 0)
    {
      send_answer_chunk(channel_mask, this_cmd, 0);
      send_answer_chunk(channel_mask, ":no slaves found", 1);
      g_active_slave = 0;
    } else
    {
      if (g_active_slave == 0)
      {
        send_answer_chunk(channel_mask, "", 1);
        handle_task_next(channel_mask);
      }

      if (g_sa_list[g_active_slave].found_ == 0) // old active slave is not found on the bus anymore.
      { // thus select next slave on the bus
        send_answer_chunk(channel_mask, "", 1);
        handle_task_next(channel_mask);
      }
    }
    return NULL;
  }

  this_cmd = "ls"; // List Slave command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    uint8_t count_slaves = 0;

    for (uint8_t sa = 1; sa<128; sa++)
    {
      if (g_sa_list[sa].found_)
      {
        count_slaves++;
        char buf[16]; memset(buf, 0, sizeof(buf));
        int16_t spot = g_sa_list[sa].spot_;
        send_answer_chunk(channel_mask, this_cmd, 0);
        send_answer_chunk(channel_mask, ":", 0);
        uint8_to_hex(buf, sa);
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ":", 0);
        uint8_to_hex(buf, g_sa_drv_register[spot].drv_);
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);
        uint8_to_hex(buf, g_sa_drv_register[spot].raw_);
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);
        uint8_to_hex(buf, g_sa_drv_register[spot].disabled_);
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);

        send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(g_sa_drv_register[spot].drv_), 1);
      }
    }

    if (count_slaves == 0)
    {
      send_answer_chunk(channel_mask, this_cmd, 0);
      send_answer_chunk(channel_mask, ":no slaves listed", 1);
    }
    return NULL;
  }

  this_cmd = "dis"; // DIsable Slave command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    int16_t i = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
      if (*(cmd+strlen(this_cmd)+3) == ':')
      {
        i = strlen(this_cmd)+4;
      }
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    uint8_t disable = 1;
    if (i > 0)
    {
      disable = atoi(cmd+i);
    }

    char buf[16]; memset(buf, 0, sizeof(buf));
    send_answer_chunk(channel_mask, this_cmd, 0);
    send_answer_chunk(channel_mask, ":", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":", 0);

    if (g_sa_list[sa].found_) // only allow disable of discovered slaves
    {
      int16_t spot = g_sa_list[sa].spot_;
      if (disable > 0)
      {
        g_sa_drv_register[spot].disabled_ = 1;
        uint8_to_hex(buf, 1);
      } else
      {
        g_sa_drv_register[spot].disabled_ = 0;
        uint8_to_hex(buf, 0);
      }
      send_answer_chunk(channel_mask, buf, 0);
      send_answer_chunk(channel_mask, ":", 0);
      send_answer_chunk(channel_mask, "OK [i2c-stick register]", 1);
    } else
    {
      send_answer_chunk(channel_mask, "FAIL: slave not seen by scan; try scan again", 1);
    }
    return NULL;
  }

  this_cmd = "mv"; // Measure (sensor) Value command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }

    handle_cmd_mv(sa, channel_mask);
    return NULL;
  }

  this_cmd = "pwm"; // Pulse With Modulation command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t pin_no = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      const char *p = cmd+strlen(this_cmd)+1;
      if (('0' <= *p) && (*p <= '9'))
      {
        pin_no = atoi(p);
      }
    }
    if (pin_no >= 0)
    {
      const char *p = strchr(cmd+strlen(this_cmd)+1, ':');
      int16_t pwm = -1;
      if (p)
      {
        pwm = atoi(p+1);
      }
      if ((pwm >= 0) && (pwm <= 255))
      {
        hal_i2c_set_pwm(pin_no, pwm);
        send_answer_chunk(channel_mask, this_cmd, 0);
        send_answer_chunk(channel_mask, ":OK", 1);
      } else
      {
        send_answer_chunk(channel_mask, this_cmd, 0);
        send_answer_chunk(channel_mask, ":FAIL:Invalid pwm value", 1);
      }
    } else
    {
      send_answer_chunk(channel_mask, this_cmd, 0);
      send_answer_chunk(channel_mask, ":FAIL:pwm invalid pin_no", 1);
    }


    return NULL;
  }

  this_cmd = "nd"; // New Data command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }

    handle_cmd_nd(sa, channel_mask);
    return NULL;
  }

  this_cmd = "as"; // Active Slave command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    send_answer_chunk(channel_mask, "as:", 0);
    char buf[16]; memset(buf, 0, sizeof(buf));
    send_answer_chunk(channel_mask, bytetohex(g_active_slave), 0);
    send_answer_chunk(channel_mask, ":", 0);

    int16_t spot = g_sa_list[g_active_slave].spot_;

    send_answer_chunk(channel_mask, bytetostr(g_sa_drv_register[spot].drv_), 0);
    send_answer_chunk(channel_mask, ",", 0);
    send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(g_sa_drv_register[spot].drv_), 1);
    return NULL;
  }

  this_cmd = "sn"; // Serial Number command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }

    handle_cmd_sn(sa, channel_mask);
    return NULL;
  }

  this_cmd = "cs"; // Config Slave command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    uint8_t i = 0;
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    handle_cmd_cs(sa, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "+cs:"; // Config Slave command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = atohex8(cmd+strlen(this_cmd));
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    uint8_t i = strlen(this_cmd);
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    handle_cmd_cs_write(sa, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "mr"; // Memory Read command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    uint8_t i = 0;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
      i += strlen(this_cmd)+1;
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    handle_cmd_mr(sa, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "mw"; // Memory Write command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    uint8_t i = 0;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
      i += strlen(this_cmd)+1;
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    handle_cmd_mw(sa, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "is"; // IS likely the correct product for this driver command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    if (sa < 0)
    {
      sa = g_active_slave;
    }

    handle_cmd_is(sa, channel_mask);
    return NULL;
  }

  this_cmd = "raw"; // read raw sensor data command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t sa = -1;
    if (cmd[strlen(this_cmd)] == ':')
    {
      sa = atohex8(cmd+strlen(this_cmd)+1);
    }
    uint8_t i = 0;
    if (sa < 0)
    {
      sa = g_active_slave;
    }
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    handle_cmd_raw(sa, channel_mask);
    return NULL;
  }


  this_cmd = "ca"; // Config Application command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t app_id = atohex8(cmd+strlen(this_cmd));
    if (app_id < 0)
    {
      app_id = g_app_id;
    }
    uint8_t i = 0;
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    cmd_ca(app_id, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "+ca:"; // Config Application command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t app_id = atohex8(cmd+strlen(this_cmd));
    uint8_t i = 0;
    if (app_id < 0)
    {
      app_id = g_app_id;
    }
    for (; i<strlen(cmd); i++)
    {
      if (cmd[i] == ':')
      {
        i++;
        break;
      }
    }

    cmd_ca_write(app_id, channel_mask, cmd+i);
    return NULL;
  }

  this_cmd = "app"; // APPlication command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    send_answer_chunk(channel_mask, "app:", 0);
    char buf[8]; memset(buf, 0, sizeof(buf));
    uint8_to_hex(buf, g_app_id);
    send_answer_chunk(channel_mask, buf, 1);
    return NULL;
  }

  this_cmd = "+app:"; // APPlication command
  if (!strncmp(this_cmd, cmd, strlen(this_cmd)))
  {
    int16_t app_id = atohex8(cmd+strlen(this_cmd));

    send_answer_chunk(channel_mask, "+app:", 0);

    if (app_id < 0)
    {
      send_answer_chunk(channel_mask, "FAILED", 1);
    } else
    {
      g_app_id = app_id;
      char buf[8]; memset(buf, 0, sizeof(buf));
      uint8_to_hex(buf, app_id);
      send_answer_chunk(channel_mask, buf, 0);
      send_answer_chunk(channel_mask, ":STARTED", 1);
    }
    return NULL;
  }

  return cmd;
}


void
handle_cmd_mv(uint8_t sa, uint8_t channel_mask)
{
  float mv_list[768+1];
  uint16_t mv_count = sizeof(mv_list)/sizeof(mv_list[0]);
  char buf[16];
  const char *error_message = NULL;
  uint32_t time_stamp = hal_get_millis();

  send_answer_chunk(channel_mask, "mv:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!g_sa_list[sa].found_)
  { // not found!
    uint32_to_dec(buf, time_stamp, 8);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":", 0);
    send_answer_chunk(channel_mask, "FAIL: Slave not found; try scan command!", 1);
    return;
  }

  if (cmd_mv(sa, mv_list, &mv_count, &error_message) == 0)
  {
    uint32_to_dec(buf, time_stamp, 8);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ":", 0);
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }
  time_stamp = hal_get_millis(); // update timestamp when data is available.
  uint32_to_dec(buf, time_stamp, 8);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);
  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }
  if (mv_count == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: local buffer not big enough", 1);
    return;
  }

  if ((g_config_host & 0x000F) == HOST_CFG_FORMAT_DEC)
  {
    for (int16_t i=0; i<mv_count; i++)
    {
      memset(buf, 0, sizeof(buf));
      const char *p = my_dtostrf(mv_list[i], 10, 2, buf);
      while (*p == ' ') p++; // remove leading space
      if ((p - buf) > 10) p = "NaN"; // should never ever happen!

      if (i < (mv_count - 1))
      { // not yet last element...
        send_answer_chunk(channel_mask, p, 0);
        send_answer_chunk(channel_mask, ",", 0);
      } else
      { // last element...
        send_answer_chunk(channel_mask, p, 1);
      }
    }
  }

  if ((g_config_host & 0x000F) == HOST_CFG_FORMAT_HEX)
  {
    char buf[16]; memset(buf, 0, sizeof(buf));

    sprintf(buf, "HEX:%d:", 32);
    send_answer_chunk(channel_mask, buf, 0);

    for (int16_t i=0; i<mv_count; i++)
    {
      memset(buf, 0, sizeof(buf));
      uint16_t int_value = int(mv_list[i] * 32) & 0x0FFFF;
      uint16_to_hex(buf, int_value);
      if (i < (mv_count - 1))
      { // not yet last element...
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);
      } else
      { // last element...
        send_answer_chunk(channel_mask, buf, 1);
      }
    }
  }

  if ((g_config_host & 0x000F) == HOST_CFG_FORMAT_BIN)
  {
    // send in text format the length of the byte-stream in binary format.
    char buf[16]; memset(buf, 0, sizeof(buf));
    sprintf(buf, "BIN:%d:%d", 32, (mv_count)*2);
    send_answer_chunk(channel_mask, buf, 1);

    for (int16_t i=0; i<mv_count; )
    {
      uint16_t int_value = int(mv_list[i] * 32) & 0x0FFFF;
      i++;
      send_answer_chunk_binary(channel_mask, (const char *) &int_value, sizeof(int_value), (i == mv_count));
    }
  }
}


void
handle_cmd_raw(uint8_t sa, uint8_t channel_mask)
{
  uint16_t raw_list[834]; memset(raw_list, 0, sizeof(raw_list));
  uint16_t raw_count = sizeof(raw_list)/sizeof(raw_list[0]);
  char buf[16];
  const char *error_message = NULL;

  send_answer_chunk(channel_mask, "raw:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!g_sa_list[sa].found_)
  { // not found!
    send_answer_chunk(channel_mask, "FAIL: Slave[", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, "] not found; try scan command!", 1);
    return;
  }

  if (cmd_raw(sa, raw_list, &raw_count, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }
  uint32_t time_stamp = hal_get_millis(); // update timestamp when data is available.
  uint32_to_dec(buf, time_stamp, 8);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }

  if (raw_count == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: local buffer not big enough", 1);
    return;
  }

  if ((g_config_host & 0x000F) == HOST_CFG_FORMAT_BIN)
  {
    char buf[16]; memset(buf, 0, sizeof(buf));
    sprintf(buf, "BIN:%d:%d", 1, (raw_count)*2);
    send_answer_chunk(channel_mask, buf, 1);

    send_answer_chunk_binary(channel_mask, (const char *)raw_list, raw_count*2, 1);
  } else
  { // in all other cases report in HEX
    for (int16_t i=0; i<raw_count; i++)
    {
      uint16_to_hex(buf, raw_list[i]);

      if (i < (raw_count - 1))
      { // not yet last element...
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);
      } else
      { // last element...
        send_answer_chunk(channel_mask, buf, 1);
      }
    }
  }

}


void
handle_cmd_nd(uint8_t sa, uint8_t channel_mask)
{
  uint8_t nd;
  char buf[16]; memset(buf, 0, sizeof(buf));
  const char *error_message = NULL;

  send_answer_chunk(channel_mask, "nd:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!g_sa_list[sa].found_)
  { // not found!
    send_answer_chunk(channel_mask, "FAIL: Slave[", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, "] not found; try scan command!", 1);
    return;
  }

  if (cmd_nd(sa, &nd, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }

  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }

  send_answer_chunk(channel_mask, nd ? "1" : "0", 1);
}


void
handle_cmd_sn(uint8_t sa, uint8_t channel_mask)
{
  uint16_t sn_list[4]; memset(sn_list, 0, sizeof(sn_list));
  uint16_t sn_count = sizeof(sn_list)/sizeof(sn_list[0]);
  char buf[16];
  const char *error_message = NULL;

  send_answer_chunk(channel_mask, "sn:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!g_sa_list[sa].found_)
  { // not found!
    send_answer_chunk(channel_mask, "FAIL: Slave[", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, "] not found; try scan command!", 1);
    return;
  }

  if (cmd_sn(sa, sn_list, &sn_count, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }

  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }

  if (sn_count == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: local buffer not big enough", 1);
    return;
  }

  for (int16_t i=0; i<sn_count; i++)
  {
    uint16_to_hex(buf, sn_list[i]);

    if (i < (sn_count - 1))
    { // not yet last element...
      send_answer_chunk(channel_mask, buf, 0);
      send_answer_chunk(channel_mask, "-", 0);
    } else
    { // last element...
      send_answer_chunk(channel_mask, buf, 1);
    }
  }
}


void
handle_cmd_ch(uint8_t channel_mask, const char *input)
{
  if (cmd_ch(channel_mask, input) == 0)
  {
    send_answer_chunk(channel_mask, "ch:ERROR", 1);
  }
}


void
handle_cmd_ch_write(uint8_t channel_mask, const char *input)
{
  if (cmd_ch_write(channel_mask, input) == 0)
  {
    send_answer_chunk(channel_mask, "+ch:ERROR", 1);
  }
}


void
handle_cmd_cs(uint8_t sa, uint8_t channel_mask, const char *input)
{
  if (cmd_cs(sa, channel_mask, input) == 0)
  {
    send_answer_chunk(channel_mask, "cs:FAIL: no device driver assigned", 1);
  }
}


void
handle_cmd_cs_write(uint8_t sa, uint8_t channel_mask, const char *input)
{
  if (cmd_cs_write(sa, channel_mask, input) == 0)
  {
    send_answer_chunk(channel_mask, "+cs:FAIL: no device driver assigned", 1);
  }
}



void
handle_cmd_mr(uint8_t sa, uint8_t channel_mask, const char *input)
{
  uint16_t mem_list[1024];
  uint16_t mem_start_address = 0;
  uint16_t mem_count = 1;
  uint8_t bit_per_address = 0;
  uint8_t address_increments = 0;
  char buf[16];
  const char *error_message = NULL;

  send_answer_chunk(channel_mask, "mr:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!strcmp(input, ""))
  {
    send_answer_chunk(channel_mask, "no input parameters given; try 'sos:mr'", 1);
    return;
  }

  int32_t temp = atohex16(input);
  if (temp > 0)
  {
    mem_start_address = temp;
  } else
  {
    send_answer_chunk(channel_mask, "invalid memory address given; try 'sos:mr'", 1);
    return;
  }

  const char *p = strchr(input, ',');
  if (p)
  {
    temp = atohex16(p+1);
    if (temp > 0)
    {
      mem_count = temp;
    }
  }

  if (!g_sa_list[sa].found_)
  { // not found!
    send_answer_chunk(channel_mask, "FAIL: Slave[", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, "] not found; try scan command!", 1);
    return;
  }

  if (cmd_mr(sa, mem_list, mem_start_address, mem_count, &bit_per_address, &address_increments, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }

  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }

  uint16_to_hex(buf, mem_start_address);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ",", 0);

  uint8_to_hex(buf, bit_per_address);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ",", 0);

  uint8_to_hex(buf, address_increments);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ",", 0);

  uint16_to_hex(buf, mem_count);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ",DATA,", 0);

  if (mem_count == 0)
  {
    send_answer_chunk(channel_mask, "no data", 1);
  } else
  {
    for (uint16_t i=0; i<mem_count; i++)
    {
      uint16_to_hex(buf, mem_list[i]);

      if (i < (mem_count - 1))
      { // not yet last element...
        send_answer_chunk(channel_mask, buf, 0);
        send_answer_chunk(channel_mask, ",", 0);
      } else
      { // last element...
        send_answer_chunk(channel_mask, buf, 1);
      }
    }
  }
}


void
handle_cmd_mw(uint8_t sa, uint8_t channel_mask, const char *input)
{
  uint16_t mem_list[1024];
  uint16_t mem_start_address = 0;
  uint16_t mem_count = 1;
  uint8_t bit_per_address = 0;
  uint8_t address_increments = 0;
  char buf[16];
  const char *error_message = NULL;

  send_answer_chunk(channel_mask, "mw:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  if (!g_sa_list[sa].found_)
  { // not found!
    send_answer_chunk(channel_mask, "FAIL: Slave[", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, "] not found; try scan command!", 1);
    return;
  }


  const char *p = input;
  int32_t temp = atohex16(p);
  if (temp > 0)
  {
    mem_start_address = temp;
  } else
  {
    send_answer_chunk(channel_mask, "invalid memory address given; try 'sos:mw'", 1);
    return;
  }

  uint16_t i=0;
  for (; i<(sizeof(mem_list)/sizeof(mem_list[0])); i++)
  {
    p = strchr(p, ',');
    if (p == NULL)
    {
      break;
    }
    p++; // skip the ',' itself
    int32_t temp = atohex16(p);
    if (temp < 0)
    {
      break;
    }
    mem_list[i] = temp;
  }
  mem_count = i;

  if (mem_count == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no data given", 1);
    return;
  }

  if (cmd_mw(sa, mem_list, mem_start_address, mem_count, &bit_per_address, &address_increments, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "FAIL: no device driver assigned", 1);
    return;
  }

  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }

  send_answer_chunk(channel_mask, "OK", 1);
}


void
handle_cmd_is(uint8_t sa, uint8_t channel_mask)
{
  char buf[16];
  const char *error_message = NULL;
  uint8_t is_ok = 0;

  send_answer_chunk(channel_mask, "is:", 0);
  uint8_to_hex(buf, sa);
  send_answer_chunk(channel_mask, buf, 0);
  send_answer_chunk(channel_mask, ":", 0);

  int16_t spot = g_sa_list[sa].spot_;
  uint8_t drv = g_sa_drv_register[spot].drv_;

  if (cmd_is(sa, drv, &is_ok, &error_message) == 0)
  {
    send_answer_chunk(channel_mask, "0:FAIL: no device driver assigned", 1);
    return;
  }
  if (error_message != NULL)
  {
    send_answer_chunk(channel_mask, "0:FAIL: ", 0);
    send_answer_chunk(channel_mask, error_message, 1);
    return;
  }
  if (is_ok)
  {
    send_answer_chunk(channel_mask, "1", 1);
  } else
  {
    send_answer_chunk(channel_mask, "0", 1);
  }
}


// host only commands

uint8_t
cmd_ch(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));
  const char *value = NULL;

  send_answer_chunk(channel_mask, "ch:FORMAT=", 0);
  uint8_t fmt = g_config_host & 0x000F;
  switch(fmt)
  {
    case HOST_CFG_FORMAT_DEC:
      value = "DEC";
      break;
    case HOST_CFG_FORMAT_HEX:
      value = "HEX";
      break;
    case HOST_CFG_FORMAT_BIN:
      value = "BIN";
      break;
    default:
      value = "Unknown";
  }
  sprintf(buf, "%d(%s)", fmt, value);
  send_answer_chunk(channel_mask, buf, 1);

  send_answer_chunk(channel_mask, "ch:I2C_FREQ=", 0);
  uint8_t freq = (g_config_host & 0x00F0) >> 4;
  switch(freq)
  {
    case HOST_CFG_I2C_F100k:
      value = "100kHz";
      break;
    case HOST_CFG_I2C_F400k:
      value = "400kHz";
      break;
    case HOST_CFG_I2C_F1M:
      value = "1MHz";
      break;
    case HOST_CFG_I2C_F50k:
      value = "50kHz";
      break;
    case HOST_CFG_I2C_F20k:
      value = "20kHz";
      break;
    case HOST_CFG_I2C_F10k:
      value = "10kHz";
      break;
    default:
      value = "Unknown";
  }
  sprintf(buf, "%d(%s)", freq, value);
  send_answer_chunk(channel_mask, buf, 1);

  for (uint16_t spot=1; spot<MAX_SA_DRV_REGISTRATIONS; spot++)
  {
    uint8_t sa = g_sa_drv_register[spot].sa_;
    uint8_t drv = g_sa_drv_register[spot].drv_;

    if ((sa == 0) || (drv == 0))
    { // skip empty spots
      continue;
    }
    send_answer_chunk(channel_mask, "ch:SA_DRV=", 0);
    uint8_to_hex(buf, sa);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ",", 0);
    uint8_to_hex(buf, drv);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ",", 0);
    send_answer_chunk(channel_mask, i2c_stick_get_drv_name_by_drv(drv), 1);
  }

  for (uint16_t drv=0; drv<=255; drv++)
  {
    const char *drv_name = i2c_stick_get_drv_name_by_drv(drv);
    if (!strcasecmp(drv_name, "Unknown"))
    { // skip the unknown drv id's
      continue;
    }
    send_answer_chunk(channel_mask, "ch:DRV=", 0);
    uint8_to_hex(buf, drv);
    send_answer_chunk(channel_mask, buf, 0);
    send_answer_chunk(channel_mask, ",", 0);
    send_answer_chunk(channel_mask, drv_name, 1);
  }

  return 1;
}


void
i2c_stick_set_i2c_clock_frequency(uint16_t enum_freq)
{
  g_config_host &= ~0x00F0;
  g_config_host |= ((enum_freq << 4) & 0x00F0);

  // apply the configuration
  uint32_t f = 100000;
  switch (enum_freq)
  {
    case HOST_CFG_I2C_F100k:
      f = 100000;
      break;
    case HOST_CFG_I2C_F400k:
      f = 400000;
      break;
    case HOST_CFG_I2C_F1M:
      f = 1000000;
      break;
    case HOST_CFG_I2C_F50k:
      f = 50000;
      break;
    case HOST_CFG_I2C_F20k:
      f = 20000;
      break;
    case HOST_CFG_I2C_F10k:
      f = 10000;
      break;
    default:
      f = 100000;
  }
  hal_i2c_set_clock_frequency(f);
}


uint16_t
i2c_stick_get_i2c_clock_frequency()
{
  return (g_config_host >> 4) & 0x000F;
}


uint8_t
cmd_ch_write(uint8_t channel_mask, const char *input)
{
  char buf[16]; memset(buf, 0, sizeof(buf));

  const char *var_name = "FORMAT=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *p = input+strlen(var_name);
    uint16_t value = 0;
    uint8_t valid = false;

    if (('0' <= p[0]) && (p[0] <= '9'))
    {
      value = atoi(p);
      if (value < 16)
      {
        valid = true;
      }
    }
    if (!valid)
    {
      if (!strcmp(p, "DEC"))
      {
        value = HOST_CFG_FORMAT_DEC;
        valid = true;
      }
      else if (!strcmp(p, "HEX"))
      {
        value = HOST_CFG_FORMAT_HEX;
        valid = true;
      }
      else if (!strcmp(p, "BIN"))
      {
        value = HOST_CFG_FORMAT_BIN;
        valid = true;
      }
    }
    if (!valid)
    {
      send_answer_chunk(channel_mask, "+ch:", 0);
      send_answer_chunk(channel_mask, input, 0);
      send_answer_chunk(channel_mask, ":ERROR: Invalid value", 1);
      return 0;
    }
    g_config_host &= ~0x000F;
    g_config_host |= (value & 0x000F);
    send_answer_chunk(channel_mask, "+ch:OK [host-register]", 1);
    return 1;
  }

  var_name = "I2C_FREQ=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *p = input+strlen(var_name);
    uint16_t value = 0;
    uint8_t valid = false;

    if (('0' <= p[0]) && (p[0] <= '9'))
    {
      value = atoi(p);
      if (value < 16)
      {
        valid = true;
      }
    }
    if (!valid)
    {
      if (!strcmp(p, "F100k"))
      {
        value = HOST_CFG_I2C_F100k;
        valid = true;
      }
      else if (!strcmp(p, "F400k"))
      {
        value = HOST_CFG_I2C_F400k;
        valid = true;
      }
      else if (!strcmp(p, "F1M"))
      {
        value = HOST_CFG_I2C_F1M;
        valid = true;
      }
      else if (!strcmp(p, "F50k"))
      {
        value = HOST_CFG_I2C_F50k;
        valid = true;
      }
      else if (!strcmp(p, "F20k"))
      {
        value = HOST_CFG_I2C_F20k;
        valid = true;
      }
      else if (!strcmp(p, "F10k"))
      {
        value = HOST_CFG_I2C_F10k;
        valid = true;
      }
    }
    if (!valid)
    {
      send_answer_chunk(channel_mask, "+ch:", 0);
      send_answer_chunk(channel_mask, input, 0);
      send_answer_chunk(channel_mask, ":ERROR: Invalid value", 1);
      return 0;
    }
    i2c_stick_set_i2c_clock_frequency(value);

    send_answer_chunk(channel_mask, "+ch:OK [host-register]", 1);
    return 1;
  }

  var_name = "SA_DRV=";
  if (!strncmp(var_name, input, strlen(var_name)))
  {
    const char *p = input+strlen(var_name);
    uint16_t value = 0;
    uint8_t valid = true;

    uint8_t do_remove = false;
    if (*p == '-')
    {
      do_remove = true;
    } else if (*p != '+')
    {
      valid = false;
    }

    int16_t sa = atohex8(p+1);
    if ((sa < 0) || (sa >= 0x80) || (p[3] != ','))
    {
      valid = false;
    }

    int16_t drv = atohex8(p+4);
    if (drv < 0)
    { // not a hex number! ==> try if it is a drv_name...
      drv = i2c_stick_get_drv_by_drv_name(p+4);
    }
    if ((drv <= 0) || (drv >= 64))
    {
      valid = false;
    }
    if (!valid)
    {
      send_answer_chunk(channel_mask, "+ch:FAIL syntax error or driver unknown", 1);
      return 0;
    }
    i2c_stick_register_driver(sa, drv);

    if (do_remove)
    {
      for (uint16_t spot=1; spot<MAX_SA_DRV_REGISTRATIONS; spot++)
      {
        if ((sa == g_sa_drv_register[spot].sa_) && (drv == g_sa_drv_register[spot].drv_))
        { // free up the spot...
          g_sa_drv_register[spot].sa_ = 0;
          g_sa_drv_register[spot].drv_ = 0;
        }
      }
    }

    send_answer_chunk(channel_mask, "+ch:OK [host-register]", 1);
    return 1;
  }


  return 0;
}

// end of host only commands

uint8_t
cmd_ca(uint8_t app_id, uint8_t channel_mask, const char *input)
{
  switch(app_id)
  {
    case APP_NONE:
      break;
    default:
      return 0;
  }
  return app_id;
}


uint8_t
cmd_ca_write(uint8_t app_id, uint8_t channel_mask, const char *input)
{
  // 1. all is specific for each application
  switch(app_id)
  {
    case APP_NONE:
      break;
    default:
      return 0;
  }
  return app_id;
}



void
handle_cmd_sos(uint8_t channel_mask, const char *input)
{
  if (*input == ':')
  {
    const char *this_cmd = ":i2c";
    if (!strncmp(this_cmd, input, strlen(this_cmd)))
    {
      send_answer_chunk(channel_mask, "I2C command format:", 1);
      send_answer_chunk(channel_mask, "1] WRITE         : 'i2c:<sa>:W<byte#0><byte#1>...' bytes in hex format", 1);
      send_answer_chunk(channel_mask, "2] READ          : 'i2c:<sa>:R<amount of bytes to read>' amount in decimal format", 1);
      send_answer_chunk(channel_mask, "3] ADDRESSED READ: 'i2c:<sa>:W<byte#0><byte#1>R<amount of bytes to read>...' bytes in hex format, amount in decimal", 1);
      send_answer_chunk(channel_mask, "<sa> Slave Address(7-bit) in hex format", 1);
      return;
    }
    this_cmd = ":ch";
    if (!strncmp(this_cmd, input, strlen(this_cmd)))
    {
      send_answer_chunk(channel_mask, "Configure Host command format:", 1);
      send_answer_chunk(channel_mask, "1] set the output format:", 1);
      send_answer_chunk(channel_mask, "    - +ch:FORMAT=DEC", 1);
      send_answer_chunk(channel_mask, "    - +ch:FORMAT=HEX", 1);
      send_answer_chunk(channel_mask, "    - +ch:FORMAT=BIN", 1);
      send_answer_chunk(channel_mask, "2] set the I2C frequency:", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F100k", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F400k", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F1M", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F50k", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F20k", 1);
      send_answer_chunk(channel_mask, "    - +ch:I2C_FREQ=F10k", 1);
      return;
    }
    this_cmd = ":dis";
    if (!strncmp(this_cmd, input, strlen(this_cmd)))
    {
      send_answer_chunk(channel_mask, "disable a slave:", 1);
      send_answer_chunk(channel_mask, "  - dis   (disable active slave)", 1);
      send_answer_chunk(channel_mask, "  - dis:33", 1);
      send_answer_chunk(channel_mask, "  - dis:33:1", 1);
      send_answer_chunk(channel_mask, "enable a slave:", 1);
      send_answer_chunk(channel_mask, "  - dis:33:0", 1);
      return;
    }
    this_cmd = ":mr";
    if (!strncmp(this_cmd, input, strlen(this_cmd)))
    {
      send_answer_chunk(channel_mask, "read memory:", 1);
      send_answer_chunk(channel_mask, "  - mr:33:2400,0100   ==> read from slave 0x33 at address 0x2400 0x0100 (or 256) addresses", 1);
      send_answer_chunk(channel_mask, "  - mr:33:2400        ==> read from slave 0x33 at address 0x2400 1 address", 1);
      send_answer_chunk(channel_mask, "Note: everything required to read is handled by firmware, regardless if it is EEPROM, RAM, ROM, Registers, ...", 1);
      return;
    }
    this_cmd = ":mw";
    if (!strncmp(this_cmd, input, strlen(this_cmd)))
    {
      send_answer_chunk(channel_mask, "write memory:", 1);
      send_answer_chunk(channel_mask, "  - mw:33:2400,1234       ==> Write the value 0x1234 to slave 0x33 at address 0x2400", 1);
      send_answer_chunk(channel_mask, "  - mw:33:2400,1234,5678  ==> Write the value 0x1234 to slave 0x33 at address 0x2400", 1);
      send_answer_chunk(channel_mask, "                              and the value 0x5678 to address 0x2401", 1);
      send_answer_chunk(channel_mask, "  - mw:33:2400            ==> no write data supplied, nothing is written", 1);
      send_answer_chunk(channel_mask, "Note: Everything required to write is handled by firmware, regardless if it is EEPROM, RAM, ROM, Registers, ...", 1);
      send_answer_chunk(channel_mask, "Note2: It might fail due to writing permissions.", 1);
      send_answer_chunk(channel_mask, "       However a customer/application unlock instruction will be issued by the firmware when required", 1);
      return;
    }
    send_answer_chunk(channel_mask, "no SOS available for command '", 0);
    send_answer_chunk(channel_mask, input, 0);
    send_answer_chunk(channel_mask, "'", 1);
    return;
  }


  send_answer_chunk(channel_mask, "sos:<cmd>", 1);
  send_answer_chunk(channel_mask, "", 1);
  send_answer_chunk(channel_mask, "example: 'sos:i2c'", 1);
}


#ifdef __cplusplus
}
#endif

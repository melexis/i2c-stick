/* 
   Driver: mlx90632-driver-float
   Version: V0.5.1
   Date: 17 May 2021
*/
/*
 Hardware Abstraction Layer (HAL)
 fake implementation for offline validation purposes.
*/
#include <Arduino.h>
#include "i2c_stick.h"
#include "i2c_stick_arduino.h"


#include "mlx90632_advanced.h"
#include "mlx90632_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


int32_t
_mlx90632_i2c_read_block(uint8_t slave_address, uint16_t register_address, uint16_t *value, uint16_t size)
{
  int16_t ack = 0;
  int16_t cnt = 0;
  int16_t n = 0;
  uint16_t *p;

  p = value;

  for (cnt = 2*size; cnt > 0; cnt -= READ_BUFFER_SIZE)
  {
    WIRE.beginTransmission(slave_address);
    WIRE.write(register_address >> 8);
    WIRE.write(register_address & 0x00FF);
    ack = WIRE.endTransmission(false);     // repeated start
    if (ack != 0x00)
    {
        return -1;
    }
    n = cnt > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : cnt;
    register_address += (n/2);
    WIRE.requestFrom((int)slave_address, (int)n);
    for(; n>0; n-=2)
    {
      uint16_t val = WIRE.read();
      val <<= 8;
      val |= WIRE.read();
      *p++ = val;
    }
  }

  ack = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040  
  if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif

  if (ack != 0x00)
  {
      return -1;
  }

  return 0;
}


int32_t
_mlx90632_i2c_write(uint8_t slave_address, uint16_t register_address, uint16_t value)
{
  WIRE.beginTransmission(slave_address);
  uint8_t register_address_MSB = uint8_t(register_address >> 8);
  WIRE.write(register_address_MSB);           
  WIRE.write(uint8_t(register_address & 0xFF));
  WIRE.write(uint8_t(value >> 8));           
  WIRE.write(uint8_t(value & 0xFF));
  int32_t r = WIRE.endTransmission();
#ifdef ARDUINO_ARCH_RP2040  
  if (r == 4) r = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  if (register_address_MSB == 0x24)
  {
    _usleep (10000, 10000);
  }
  return r;
}


void
_usleep(int min_range, int max_range)
{
  delay (min_range / 1000);
}


#ifdef __cplusplus
}
#endif

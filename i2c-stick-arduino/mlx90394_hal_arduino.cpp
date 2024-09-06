/**
 * @copyright (C) 2024 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <Arduino.h>

#include "i2c_stick_arduino.h"
#include "mlx90394_hal.h"


void
mlx90394_i2c_init()
{   
  WIRE.endTransmission();
}


int
mlx90394_i2c_direct_read(uint8_t sa, uint16_t *data, uint8_t count)
{
  int ack = 0;

  WIRE.endTransmission();
  mlx90394_delay_us(5);

  WIRE.beginTransmission(sa);
  ack = WIRE.requestFrom(sa, count, (uint8_t)(true));
  Serial.printf("request from ret: %d\n", ack);
  for (uint8_t i=0; i<count; i++)
  {
    uint8_t val = WIRE.read();
    data[i] = val;
  }
  return 0;
}


int
mlx90394_i2c_addressed_read(uint8_t sa, uint8_t read_address, uint16_t *data, uint8_t count)
{
  int ack = 0;

  WIRE.endTransmission();
  mlx90394_delay_us(5);

  WIRE.beginTransmission(sa);
  WIRE.write(uint8_t(read_address));
  ack = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040  
  if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  if (ack != 0x00)
  {
      return -1;
  }
  ack = WIRE.requestFrom(sa, count, (uint8_t)(true));
  if (ack != count)
  {
    return -2;
  }

  for (uint8_t i=0; i<count; i++)
  {
    uint8_t val = WIRE.read();
    data[i] = val;
  }
  return 0;
}


void
mlx90394_i2c_set_clock_frequency(int freq)
{
  WIRE.end();          // some MCU cannot change the clock while I2C is active.
  WIRE.setClock(freq); // RP2040 requires first to set the clock
  WIRE.begin();
  WIRE.setClock(freq); // NRF52840 requires first begin, then set clock.
}


int
mlx90394_i2c_addressed_write(uint8_t sa, uint8_t write_address, uint8_t data)
{
  int ack = 0;

  WIRE.endTransmission();
  mlx90394_delay_us(5);

  WIRE.beginTransmission(sa);
  WIRE.write(write_address);
  WIRE.write(data);
  ack = WIRE.endTransmission();
#ifdef ARDUINO_ARCH_RP2040
  if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
  if (ack != 0x00)
  {
    return -1;
  }

  return 0;
}


void
mlx90394_delay_us(int32_t delay_us)
{
  delayMicroseconds(delay_us);
}

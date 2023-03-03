/**
 * @copyright (C) 2017 Melexis N.V.
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
#include <Wire.h>
#include "mlx90641_api.h"
#include "mlx90641_i2c_driver.h"

#include "i2c_stick.h"
#include "i2c_stick_arduino.h"


void MLX90641_I2CInit()
{
  delayMicroseconds(5);
  WIRE.endTransmission();
}


int MLX90641_I2CGeneralReset(void)
{
    int ack;

    WIRE.endTransmission();
    delayMicroseconds(5);

    WIRE.beginTransmission(0x00);
    WIRE.write(0x06);
    ack = WIRE.endTransmission();
#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif

    if (ack != 0x00)
    {
        return -1;
    }

    delayMicroseconds(50);
    return 0;
}


int MLX90641_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    int16_t ack = 0;
    int16_t cnt = 0;
    int16_t n = 0;
    uint16_t *p;

    p = data;

    WIRE.endTransmission();
    delayMicroseconds(5);

    for (cnt = 2*nMemAddressRead; cnt > 0; cnt -= READ_BUFFER_SIZE)
    {
      WIRE.beginTransmission(slaveAddr);
      WIRE.write(startAddress >> 8);
      WIRE.write(startAddress & 0x00FF);
      ack = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040  
      if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
      if (ack != 0x00)
      {
          return -1;
      }
      n = cnt > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : cnt;
      startAddress += (n/2);
      WIRE.requestFrom((int)slaveAddr, (int)n);
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


void MLX90641_I2CFreqSet(int freq)
{
    WIRE.end();          // some MCU cannot change the clock while I2C is active.
    WIRE.setClock(freq); // RP2040 requires first to set the clock
    WIRE.begin();
    WIRE.setClock(freq); // NRF52840 requires first begin, then set clock.
}


int MLX90641_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    uint16_t dataCheck;
    WIRE.beginTransmission(slaveAddr);
    uint8_t writeAddress_MSB = uint8_t(writeAddress >> 8);
    WIRE.write(uint8_t(writeAddress_MSB));
    WIRE.write(uint8_t(writeAddress & 0xFF));
    WIRE.write(uint8_t(data >> 8));
    WIRE.write(uint8_t(data & 0xFF));
    int16_t ack = WIRE.endTransmission();
#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
    if (ack != 0x00)
    {
        return -1;
    }
    if (writeAddress_MSB == 0x24) // write to EEPROM?
    {
        delay(10); // 10 ms write time
    }

    MLX90641_I2CRead(slaveAddr, writeAddress, 1, &dataCheck);

    if ( dataCheck != data)
    {
        return -2;
    }

    return 0;
}

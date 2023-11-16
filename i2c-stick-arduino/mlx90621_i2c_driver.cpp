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
#include "mlx90621_i2c_driver.h"

#include "i2c_stick.h"
#include "i2c_stick_arduino.h"


void MLX90621_I2CInit()
{   
  delayMicroseconds(5);
  WIRE.endTransmission();
}

int MLX90621_I2CReadEEPROM(uint8_t slaveAddr, uint8_t startAddress, uint16_t nMemAddressRead, uint8_t *data)
{
    int16_t ack = 0;
    int16_t cnt = 0;
    int16_t n = 0;
    uint8_t *p;

    p = data;

    WIRE.endTransmission();
    delayMicroseconds(5);

    for (cnt = nMemAddressRead; cnt > 0; cnt -= READ_BUFFER_SIZE)
    {
      WIRE.beginTransmission(slaveAddr);
      WIRE.write(startAddress);
      ack = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040  
      if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
      if (ack != 0x00)
      {
        return -1;
      }
      n = cnt > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : cnt;
      startAddress += n;
      WIRE.requestFrom((int)slaveAddr, (int)n);
      for(uint16_t i=0; i<n; i++)
      {
        uint8_t val = WIRE.read();
        *p++ = val;
      }
      ack = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040  
      if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif

      if (ack != 0x00)
      {
        return -1;
      }
    }

    return 0;
} 


int MLX90621_I2CRead(uint8_t slaveAddr,uint8_t command, uint8_t startAddress, uint8_t addressStep, uint8_t nMemAddressRead, uint16_t *data)
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
      n = cnt > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : cnt;
      WIRE.beginTransmission(slaveAddr);
      WIRE.write(command);
      WIRE.write(startAddress);
      WIRE.write(addressStep);
      WIRE.write(nMemAddressRead);
      ack = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040  
      if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
      if (ack != 0x00)
      {
          return -1;
      }
      WIRE.requestFrom((int)slaveAddr, (int)n);
      startAddress += (n/2);
      for(; n>0; n-=2)
      {
        uint16_t low_byte = WIRE.read();
        uint16_t high_byte = WIRE.read();
        *p = (low_byte | (high_byte << 8));
        p++;
      }
      ack = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040  
      if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif

      if (ack != 0x00)
      {
        return -1;
      }
    }


    return 0;
} 


void MLX90621_I2CFreqSet(int freq)
{
    WIRE.end();          // some MCU cannot change the clock while I2C is active.
    WIRE.setClock(freq); // RP2040 requires first to set the clock
    WIRE.begin();
    WIRE.setClock(freq); // NRF52840 requires first begin, then set clock.
}


int MLX90621_I2CWrite(uint8_t slaveAddr, uint8_t command, uint8_t checkValue, uint16_t data)
{
    char cmd[5] = {0,0,0,0,0};
    cmd[0] = command;
    cmd[2] = data & 0x00FF;
    cmd[1] = cmd[2] - checkValue;
    cmd[4] = data >> 8;
    cmd[3] = cmd[4] - checkValue;

    uint16_t dataCheck;
    WIRE.beginTransmission(slaveAddr);
    for (uint8_t i=0; i<5; i++)
    {
        WIRE.write(cmd[i]);
    }
    int16_t ack = WIRE.endTransmission();
    delayMicroseconds(5);

#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
    if (ack != 0x00)
    {
        return -1;
    }
    // if (writeAddress_MSB == 0x24) // write to EEPROM?
    // {
    //     delay(10); // 10 ms write time
    // }

    MLX90621_I2CRead(slaveAddr, 0x02, 0x8F+command, 0, 1, &dataCheck);
    
    if ( dataCheck != data)
    {
        return -2;
    }    
    
    return 0;
}

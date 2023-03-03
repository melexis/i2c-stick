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
#include "i2c_stick.h"
//#include <unistd.h>
#include "mlx90614_smbus_driver.h"

#include "i2c_stick_arduino.h"


uint8_t Calculate_PEC(uint8_t, uint8_t);
void WaitEE(uint16_t ms);

void MLX90614_SMBusInit()
{   
  WIRE.endTransmission();
}

int MLX90614_SMBusRead(uint8_t slaveAddr, uint8_t readAddress, uint16_t *data)
{
    uint8_t sa;                           
    int ack = 0;
    uint8_t pec;                               
    uint8_t pec_read;                               

    sa = (slaveAddr << 1);
    
    WIRE.endTransmission();
    delayMicroseconds(5);    

    WIRE.beginTransmission(slaveAddr);
    pec = Calculate_PEC(0, sa);
    WIRE.write(uint8_t(readAddress));           
    pec = Calculate_PEC(pec, uint8_t(readAddress));
    ack = WIRE.endTransmission(false);     // repeated start
#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
    if (ack != 0x00)
    {
        return -1;
    }
    WIRE.requestFrom(slaveAddr, (uint8_t)(3), (uint8_t)(true));
    pec = Calculate_PEC(pec, sa|1);
    uint8_t val = WIRE.read();
    pec = Calculate_PEC(pec, val);
    *data = (uint16_t)(val);
    val = WIRE.read();
    pec = Calculate_PEC(pec, val);
    *data |= ((uint16_t)(val)<<8);    
    pec_read = WIRE.read();
        
    ack = WIRE.endTransmission();     // stop transmitting
#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
   if (ack != 0x00)
    {
        return -1; 
    }          

    if (pec != pec_read)
    {
        return -2;
    }

    return 0;
} 

void MLX90614_SMBusFreqSet(int freq)
{
    WIRE.end();          // some MCU cannot change the clock while I2C is active.
    WIRE.setClock(freq); // RP2040 requires first to set the clock
    WIRE.begin();
    WIRE.setClock(freq); // NRF52840 requires first begin, then set clock.
}

int MLX90614_SMBusWrite(uint8_t slaveAddr, uint8_t writeAddress, uint16_t data)
{
    uint8_t sa;
    int ack = 0;
    char cmd[4] = {0,0,0,0};
    static uint16_t dataCheck;
    uint8_t pec;
    
    sa = (slaveAddr << 1);
    cmd[0] = writeAddress;
    cmd[1] = data & 0x00FF;
    cmd[2] = data >> 8;
    
    pec = Calculate_PEC(0, sa);
    pec = Calculate_PEC(pec, cmd[0]);
    pec = Calculate_PEC(pec, cmd[1]);
    pec = Calculate_PEC(pec, cmd[2]);
    
    cmd[3] = pec;

    WIRE.endTransmission();
    delayMicroseconds(5);    

    WIRE.beginTransmission(slaveAddr);
    WIRE.write(cmd[0]);
    WIRE.write(cmd[1]);
    WIRE.write(cmd[2]);
    WIRE.write(cmd[3]);
    ack = WIRE.endTransmission();
#ifdef ARDUINO_ARCH_RP2040  
    if (ack == 4) ack = 0; // ignore error=4 ('other error', but I can't seem to find anything wrong; only on this MCU platform)
#endif
    if (ack != 0x00)
    {
        return -1;
    }         
    
    WaitEE(10);
    
    MLX90614_SMBusRead(slaveAddr, writeAddress, &dataCheck);
    
    if ( dataCheck != data)
    {
        return -3;
    }    
    
    return 0;
}

int MLX90614_SendCommand(uint8_t slaveAddr, uint8_t command)
{
    uint8_t sa;
    int ack = 0;
    char cmd[2]= {0,0};
    uint8_t pec;
    
    if(command != 0x60 && command != 0x61)
    {
        return -5;
    }
         
    sa = (slaveAddr << 1);
    cmd[0] = command;
        
    pec = Calculate_PEC(0, sa);
    pec = Calculate_PEC(pec, cmd[0]);
       
    cmd[1] = pec;

    WIRE.endTransmission();
    delayMicroseconds(5);    

    WIRE.beginTransmission(slaveAddr);
    WIRE.write(cmd[0]);
    WIRE.write(cmd[1]);
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


uint8_t Calculate_PEC (uint8_t initPEC, uint8_t newData)
{
    uint8_t data;
    uint8_t bitCheck;

    data = initPEC ^ newData;
    
    for (int i=0; i<8; i++ )
    {
        bitCheck = data & 0x80;
        data = data << 1;
        
        if (bitCheck != 0)
        {
            data = data ^ 0x07;
        }
        
    }
    return data;
}


void WaitEE(uint16_t ms)
{
    delay(ms);
}

#include "mlx90642_depends.h"
#include "mlx90642.h"
#include "i2c_stick_arduino.h"

#include <Arduino.h>
#include <Wire.h>


int
MLX90642_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *rData)
{
    int16_t ack = 0;
    int16_t cnt = 0;
    int16_t n = 0;
    uint16_t *p;

    p = rData;

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
      startAddress += n;
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


int MLX90642_I2CWrite(uint8_t slaveAddr, uint8_t *buffer, uint8_t bytesNum)
{
    int16_t ack = 0;
    int16_t cnt = 0;
    int16_t n = 0;
    uint16_t *p;

    WIRE.endTransmission();
    delayMicroseconds(5);

    WIRE.beginTransmission(slaveAddr);
    for (uint8_t i=0; i<bytesNum; i++)
    {
        WIRE.write(buffer[i]);
    }
    ack = WIRE.endTransmission(false); // STOP condition
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
MLX90642_Wait_ms(uint16_t time_ms)
{
    delay(time_ms);
}


// int
// MLX90642_Config(uint8_t slaveAddr, uint16_t writeAddress, uint16_t wData)
// {
// 	uint16_t write_buffer[3];
//     memset(write_buffer, 0, sizeof(write_buffer));

// 	write_buffer[0] = MLX90642_CONFIG_OPCODE;
// 	write_buffer[1] = writeAddress;
// 	write_buffer[2] = wData;

//     return MLX90642_I2CWrite(slaveAddr, write_buffer, 3);
// }


// int
// MLX90642_I2CCmd(uint8_t slaveAddr, uint16_t i2c_cmd)
// {
//     uint16_t write_buffer[2];
//     memset(write_buffer, 0, sizeof(write_buffer));

//     write_buffer[0] = MLX90642_CMD_OPCODE;
//     write_buffer[1] = i2c_cmd;

//     return MLX90642_I2CWrite(slaveAddr, write_buffer, 2);
// }


// // missing in mlx90642.h
// #define MLX90642_WAKEUP_CMD 0x0057

// int
// MLX90642_WakeUp(uint8_t slaveAddr)
// {
//     uint16_t write_buffer[1];
//     memset(write_buffer, 0, sizeof(write_buffer));

//     write_buffer[0] = MLX90642_WAKEUP_CMD;

//     return MLX90642_I2CWrite(slaveAddr, write_buffer, 1);
// }
